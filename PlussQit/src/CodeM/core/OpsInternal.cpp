#include "CodeM/core/OpsInternal.h"

// OpsInternal.cpp defines Series member functions; it requires the full Series type.
#include "CodeM/core/Series.h"

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
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/ViewInvariants.h"
#include "CodeM/core/SeriesTape.h"
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
#include "pluss/tils/aux/rng_seed.h"

#ifdef PLUSS_ENABLE_TORCH
#include "pluss/codem/torch_interop.h"
#endif

namespace pluss::codem::ops_internal {

namespace {

thread_local std::vector<Platform> g_platform_scope_stack;

const char* platform_name_(Platform p) noexcept {
  switch (p) {
    case Platform::CPU: return "CPU";
    case Platform::CUDA: return "CUDA";
    case Platform::MPS: return "MPS";
    default: return "UNKNOWN";
  }
}

Platform scoped_platform_or_cpu_() noexcept {
  if (g_platform_scope_stack.empty()) {
    return Platform::CPU;
  }
  return g_platform_scope_stack.back();
}

bool parse_bool_env_(const char* name, bool default_value) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return default_value;
  }
  auto eq_lit = [&](const char* lit) noexcept {
    const char* a = raw;
    const char* b = lit;
    while (*a != '\0' && *b != '\0') {
      if (std::tolower(static_cast<unsigned char>(*a)) !=
          std::tolower(static_cast<unsigned char>(*b))) {
        return false;
      }
      ++a;
      ++b;
    }
    return *a == '\0' && *b == '\0';
  };
  if (eq_lit("1") || eq_lit("true") || eq_lit("yes") || eq_lit("on")) {
    return true;
  }
  if (eq_lit("0") || eq_lit("false") || eq_lit("no") || eq_lit("off")) {
    return false;
  }
  return default_value;
}

bool mps_cpu_fallback_enabled_() noexcept {
  return parse_bool_env_("PLUSS_MPS_ALLOW_CPU_FALLBACK", false);
}

bool mps_native_take_put_enabled_() noexcept {
  return parse_bool_env_("PLUSS_MPS_NATIVE_TAKE_PUT", true);
}

void sync_mps_alias_source_if_needed_(const Series& input) {
  if (input.defined() && input.platform() == Platform::MPS) {
    native::mps::sync_host_if_needed(input);
  }
}

bool mps_exact_f32_roundtrip_lane_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Bool:
    case ScalarType::Int8:
    case ScalarType::UInt8:
    case ScalarType::Int16:
    case ScalarType::UInt16:
      return true;
    default:
      return false;
  }
}

ScalarType stable_special_work_dtype_(const Series& input) {
  if (input.platform() == Platform::MPS) {
    return ScalarType::Float32;
  }
  if (input.scalar_type() == ScalarType::Float64) {
    return ScalarType::Float64;
  }
  return ScalarType::Float32;
}

Series stable_erfc_piecewise_(const Series& x) {
  PLUSS_CHECK(
      x.scalar_type() == ScalarType::Float32 || x.scalar_type() == ScalarType::Float64,
      "stable_erfc_piecewise_: expected float32 or float64 working dtype");
  const double tiny =
      (x.scalar_type() == ScalarType::Float64) ? 1e-300 : std::numeric_limits<float>::min();
  constexpr double erx = 8.45062911510467529297e-01;
  constexpr double pp0 = 1.28379167095512558561e-01;
  constexpr double pp1 = -3.25042107247001499370e-01;
  constexpr double pp2 = -2.84817495755985104766e-02;
  constexpr double pp3 = -5.77027029648944159157e-03;
  constexpr double pp4 = -2.37630166566501626084e-05;
  constexpr double qq1 = 3.97917223959155352819e-01;
  constexpr double qq2 = 6.50222499887672944485e-02;
  constexpr double qq3 = 5.08130628187576562776e-03;
  constexpr double qq4 = 1.32494738004321644526e-04;
  constexpr double qq5 = -3.96022827877536812320e-06;
  constexpr double pa0 = -2.36211856075265944077e-03;
  constexpr double pa1 = 4.14856118683748331666e-01;
  constexpr double pa2 = -3.72207876035701323847e-01;
  constexpr double pa3 = 3.18346619901161753674e-01;
  constexpr double pa4 = -1.10894694282396677476e-01;
  constexpr double pa5 = 3.54783043256182359371e-02;
  constexpr double pa6 = -2.16637559486879084300e-03;
  constexpr double qa1 = 1.06420880400844228286e-01;
  constexpr double qa2 = 5.40397917702171048937e-01;
  constexpr double qa3 = 7.18286544141962662868e-02;
  constexpr double qa4 = 1.26171219808761642112e-01;
  constexpr double qa5 = 1.36370839120290507362e-02;
  constexpr double qa6 = 1.19844998467991074170e-02;
  constexpr double ra0 = -9.86494403484714822705e-03;
  constexpr double ra1 = -6.93858572707181764372e-01;
  constexpr double ra2 = -1.05586262253232909814e+01;
  constexpr double ra3 = -6.23753324503260060396e+01;
  constexpr double ra4 = -1.62396669462573470355e+02;
  constexpr double ra5 = -1.84605092906711035994e+02;
  constexpr double ra6 = -8.12874355063065934246e+01;
  constexpr double ra7 = -9.81432934416914548592e+00;
  constexpr double sa1 = 1.96512716674392571292e+01;
  constexpr double sa2 = 1.37657754143519042600e+02;
  constexpr double sa3 = 4.34565877475229228821e+02;
  constexpr double sa4 = 6.45387271733267880336e+02;
  constexpr double sa5 = 4.29008140027567833386e+02;
  constexpr double sa6 = 1.08635005541779435134e+02;
  constexpr double sa7 = 6.57024977031928170135e+00;
  constexpr double sa8 = -6.04244152148580987438e-02;
  constexpr double rb0 = -9.86494292470009928597e-03;
  constexpr double rb1 = -7.99283237680523006574e-01;
  constexpr double rb2 = -1.77579549177547519889e+01;
  constexpr double rb3 = -1.60636384855821916062e+02;
  constexpr double rb4 = -6.37566443368389627722e+02;
  constexpr double rb5 = -1.02509513161107724954e+03;
  constexpr double rb6 = -4.83519191608651397019e+02;
  constexpr double sb1 = 3.03380607434824582924e+01;
  constexpr double sb2 = 3.25792512996573918826e+02;
  constexpr double sb3 = 1.53672958608443695994e+03;
  constexpr double sb4 = 3.19985821950859553908e+03;
  constexpr double sb5 = 2.55305040643316442583e+03;
  constexpr double sb6 = 4.74528541206955367215e+02;
  constexpr double sb7 = -2.24409524465858183362e+01;

  const Series absx = x.abs();
  const Series nan_mask = x.isnan();
  const Series inf_mask = x.isinf();
  const Series neg = x.lt(0.0);
  const Series zero = x.mul(0.0);
  const auto const_like = [&](double c) { return zero.add(c); };

  const Series z = absx.square();
  const Series r_small = z.mul(pp4).add(pp3).mul(z).add(pp2).mul(z).add(pp1).mul(z).add(pp0);
  const Series s_small =
      z.mul(qq5).add(qq4).mul(z).add(qq3).mul(z).add(qq2).mul(z).add(qq1).mul(z).add(1.0);
  const Series y_small_core = r_small.div(s_small);
  const Series y_small_lo = x.mul(y_small_core).add(x).mul(-1.0).add(1.0);
  const Series y_small_hi = x.mul(y_small_core).add(x.sub(0.5)).mul(-1.0).add(0.5);
  Series y_small = x.lt(0.25).where(y_small_lo, y_small_hi);
  y_small = absx.lt(std::ldexp(1.0, -56)).where(x.mul(-1.0).add(1.0), y_small);

  const Series s_mid = absx.sub(1.0);
  const Series p_mid =
      s_mid.mul(pa6).add(pa5).mul(s_mid).add(pa4).mul(s_mid).add(pa3).mul(s_mid).add(pa2).mul(s_mid).add(pa1).mul(s_mid).add(pa0);
  const Series q_mid =
      s_mid.mul(qa6).add(qa5).mul(s_mid).add(qa4).mul(s_mid).add(qa3).mul(s_mid).add(qa2).mul(s_mid).add(qa1).mul(s_mid).add(1.0);
  const Series mid_ratio = p_mid.div(q_mid);
  const Series y_mid_pos = const_like(1.0 - erx).sub(mid_ratio);
  const Series y_mid_neg = const_like(1.0 + erx).add(mid_ratio);
  const Series y_mid = neg.where(y_mid_neg, y_mid_pos);

  const Series inv_z = z.reciprocal();
  const Series r_tail_a = inv_z.mul(ra7).add(ra6).mul(inv_z).add(ra5).mul(inv_z).add(ra4).mul(inv_z).add(ra3).mul(inv_z).add(ra2).mul(inv_z).add(ra1).mul(inv_z).add(ra0);
  const Series s_tail_a = inv_z.mul(sa8).add(sa7).mul(inv_z).add(sa6).mul(inv_z).add(sa5).mul(inv_z).add(sa4).mul(inv_z).add(sa3).mul(inv_z).add(sa2).mul(inv_z).add(sa1).mul(inv_z).add(1.0);
  const Series r_tail_b = inv_z.mul(rb6).add(rb5).mul(inv_z).add(rb4).mul(inv_z).add(rb3).mul(inv_z).add(rb2).mul(inv_z).add(rb1).mul(inv_z).add(rb0);
  const Series s_tail_b = inv_z.mul(sb7).add(sb6).mul(inv_z).add(sb5).mul(inv_z).add(sb4).mul(inv_z).add(sb3).mul(inv_z).add(sb2).mul(inv_z).add(sb1).mul(inv_z).add(1.0);
  const Series use_a = absx.lt(2.8571414947509766);
  const Series r_tail = use_a.where(r_tail_a, r_tail_b);
  const Series s_tail = use_a.where(s_tail_a, s_tail_b);
  Series y_tail = absx.square().mul(-1.0).sub(0.5625).add(r_tail.div(s_tail)).exp().div(absx);
  y_tail = neg.where(y_tail.mul(-1.0).add(2.0), y_tail);
  y_tail = x.lt(-6.0).where(const_like(2.0 - tiny), y_tail);
  y_tail = x.ge(28.0).where(const_like(tiny * tiny), y_tail);

  const Series small_mask = absx.lt(0.84375);
  const Series mid_mask = absx.ge(0.84375).logical_and(absx.lt(1.25));
  Series out = small_mask.where(y_small, mid_mask.where(y_mid, y_tail));
  out = inf_mask.where(neg.where(const_like(2.0), const_like(0.0)), out);
  out = nan_mask.where(x, out);
  return out;
}

Series stable_erfc_noncpu_(const Series& input) {
  const Series x = input.to(stable_special_work_dtype_(input));
  return stable_erfc_piecewise_(x).to(input.scalar_type());
}

Series stable_erf_noncpu_(const Series& input) {
  const Series x = input.to(stable_special_work_dtype_(input));
  const Series nan_mask = x.isnan();
  const Series inf_mask = x.isinf();
  const Series neg = x.lt(0.0);
  const Series zero = x.mul(0.0);
  const auto const_like = [&](double c) { return zero.add(c); };
  const Series erfc_abs = stable_erfc_piecewise_(x.abs());
  Series out = const_like(1.0).sub(erfc_abs);
  out = neg.where(out.neg(), out);
  out = inf_mask.where(neg.where(const_like(-1.0), const_like(1.0)), out);
  out = nan_mask.where(x, out);
  return out.to(input.scalar_type());
}

void require_mps_native_dispatch_supported_(std::string_view op) {
  if (mps_cpu_fallback_enabled_()) {
    return;
  }
  // Explicit platform transfers remain available with fallback disabled; dtype
  // casts without a native MPS kernel must fail loud like other data-plane ops.
  if (op.rfind("Series::platform(", 0) == 0) {
    return;
  }
  PLUSS_FAIL(
      std::string(op) +
      ": no native MPS kernel yet for this dtype/op. "
      "CPU fallback is disabled by policy. "
      "Set PLUSS_MPS_ALLOW_CPU_FALLBACK=1 to allow bridge fallback.");
}

void require_backend_dispatch_supported_(Platform platform, std::string_view op) {
  switch (platform) {
    case Platform::CPU:
      return;
    case Platform::MPS:
      require_mps_native_dispatch_supported_(op);
      return;
    case Platform::CUDA:
      if (!native::cuda::runtime_available()) {
        PLUSS_FAIL(
            std::string(op) +
            ": CUDA runtime unavailable (" +
            native::cuda::runtime_error_cstr() + ")");
      }
      if (native::cuda::cpu_bridge_enabled()) {
        return;
      }
      PLUSS_FAIL(
          std::string(op) +
          ": no native CUDA kernel yet. CPU fallback is not enabled for CUDA backend.");
    default:
      PLUSS_FAIL(std::string(op) + ": unsupported backend platform");
  }
}

void require_mps_support_or_policy_(bool supported, std::string_view op) {
  if (supported) {
    return;
  }
  require_mps_native_dispatch_supported_(op);
}

[[noreturn]] void throw_cuda_indexing_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native indexing route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA indexing path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_indexing_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_indexing_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_scatter_gather_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native scatter/gather route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA scatter/gather path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_scatter_gather_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_scatter_gather_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_elementwise_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native elementwise route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA elementwise path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_elementwise_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_elementwise_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_reduction_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native reduction route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA reduction path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_reduction_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_reduction_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_scan_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native scan route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA scan path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_scan_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_scan_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_masking_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native masking route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA masking path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_masking_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_masking_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_bitwise_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native bitwise route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA bitwise path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_bitwise_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_bitwise_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_compose_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native compose route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA compose path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_compose_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_compose_unavailable_(op, native_enabled, native_strict, route_supported);
}

[[noreturn]] void throw_cuda_counting_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    PLUSS_FAIL(
        std::string(op) +
        ": CUDA native counting route declared but kernel is not implemented yet");
  }
  PLUSS_FAIL(
      std::string(op) +
      ": CUDA counting path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_counting_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    return cpu_route();
  }
  if (native_strict) {
    PLUSS_FAIL(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_counting_unavailable_(op, native_enabled, native_strict, route_supported);
}

} // namespace

PlatformScope::PlatformScope(Platform p) noexcept {
  g_platform_scope_stack.push_back(p);
  active_ = true;
}

PlatformScope::~PlatformScope() {
  if (!active_) {
    return;
  }
  if (!g_platform_scope_stack.empty()) {
    g_platform_scope_stack.pop_back();
  }
}

void require_same_platform(const Series& a, const Series& b, std::string_view op) {
  if (!a.defined() || !b.defined()) {
    return;
  }
  if (a.platform() == b.platform()) {
    return;
  }
  PLUSS_FAIL(
      std::string(op) + ": expected all Series on same platform, got " +
      platform_name_(a.platform()) + " and " + platform_name_(b.platform()));
}

static std::vector<std::int64_t> broadcast_out_shape_(std::span<const std::int64_t> a,
                                                      std::span<const std::int64_t> b) {
  return SeriesIterator::broadcast_shape(a, b);
}

ScalarType promoted_binary_dtype_(native::cpu::BinaryOpId op, ScalarType a, ScalarType b) {
  ScalarType dt = promote_types(a, b);

  // Match tensor-style "true division" semantics: integer/bool division
  // promotes to floating output instead of truncating integer arithmetic.
  if (op == native::cpu::BinaryOpId::Div &&
      (dt == ScalarType::Bool || dtype_of(dt).is_integral())) {
    return ScalarType::Float64;
  }

  if (dt != ScalarType::Bool) {
    return dt;
  }

  // Keep bool-only multiplication as logical masking, but route other bool
  // arithmetic ops through numeric dtypes so mask formulas work without casts.
  switch (op) {
    case native::cpu::BinaryOpId::Add:
    case native::cpu::BinaryOpId::Sub:
      return ScalarType::Int64;
    case native::cpu::BinaryOpId::Div:
    case native::cpu::BinaryOpId::Pow:
      return ScalarType::Float32;
    default:
      return dt;
  }
}

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

static Series binary_op_(native::cpu::BinaryOpId op, const Series& a0, const Series& b0) {
  require_same_platform(a0, b0, "binary op");
  ScalarType dt = promoted_binary_dtype_(op, a0.scalar_type(), b0.scalar_type());
  Series a = (a0.scalar_type()==dt) ? a0 : a0.to(dt);
  Series b = (b0.scalar_type()==dt) ? b0 : b0.to(dt);
  auto out_shape = broadcast_out_shape_(a.sizes(), b.sizes());
  Series out = Series::empty(out_shape, dt, a.platform());
  if (a.platform() == Platform::MPS) {
    if (native::mps::supports_binary(op, dt)) {
      const Series aa = a.is_contiguous() ? a : a.contiguous();
      const Series bb = b.is_contiguous() ? b : b.contiguous();
      native::mps::binary(op, aa, bb, out);
      return out;
    }
    const bool supports_f32 = native::mps::supports_binary(op, ScalarType::Float32);
    require_mps_support_or_policy_(supports_f32, "binary op");
    if (supports_f32) {
      const Series aa = a.to(ScalarType::Float32).contiguous();
      const Series bb = b.to(ScalarType::Float32).contiguous();
      Series out_f = Series::empty(out_shape, ScalarType::Float32, a.platform());
      native::mps::binary(op, aa, bb, out_f);
      return out_f.to(dt);
    }
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_binary_elementwise(a, b);
    return cuda_elementwise_route_or_bridge_(
        "binary op",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          auto fn = native::cpu::lookup_binary(op, dt);
          PLUSS_CHECK(fn != nullptr, "binary kernel missing");
          fn(a, b, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "binary op");
  auto fn = native::cpu::lookup_binary(op, dt);
  PLUSS_CHECK(fn != nullptr, "binary kernel missing");
  fn(a, b, out);
  return out;
}

static Series cmp_op_(native::cpu::BinaryOpId op, const Series& a0, const Series& b0) {
  require_same_platform(a0, b0, "comparison op");
  ScalarType dt = promote_types(a0.scalar_type(), b0.scalar_type());
  Series a = (a0.scalar_type()==dt) ? a0 : a0.to(dt);
  Series b = (b0.scalar_type()==dt) ? b0 : b0.to(dt);
  auto out_shape = broadcast_out_shape_(a.sizes(), b.sizes());
  Series out = Series::empty(out_shape, ScalarType::Bool, a.platform());
  if (a.platform() == Platform::MPS) {
    if (native::mps::supports_binary(op, dt)) {
      const Series aa = a.is_contiguous() ? a : a.contiguous();
      const Series bb = b.is_contiguous() ? b : b.contiguous();
      native::mps::binary(op, aa, bb, out);
      return out;
    }
    const bool supports_f32 = native::mps::supports_binary(op, ScalarType::Float32);
    require_mps_support_or_policy_(supports_f32, "comparison op");
    if (supports_f32) {
      const Series aa = a.to(ScalarType::Float32).contiguous();
      const Series bb = b.to(ScalarType::Float32).contiguous();
      native::mps::binary(op, aa, bb, out);
      return out;
    }
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_comparison_elementwise(a, b);
    return cuda_elementwise_route_or_bridge_(
        "comparison op",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          auto fn = native::cpu::lookup_binary(op, dt);
          PLUSS_CHECK(fn != nullptr, "compare kernel missing");
          fn(a, b, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "comparison op");
  auto fn = native::cpu::lookup_binary(op, dt);
  PLUSS_CHECK(fn != nullptr, "compare kernel missing");
  fn(a, b, out);
  return out;
}

static Series unary_op_(native::cpu::UnaryOpId op, const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    if (native::mps::supports_unary(op, a.scalar_type())) {
      const Series aa = a.is_contiguous() ? a : a.contiguous();
      native::mps::unary(op, aa, out);
      return out;
    }
    const bool supports_f32 = native::mps::supports_unary(op, ScalarType::Float32);
    require_mps_support_or_policy_(supports_f32, "unary op");
    if (supports_f32) {
      const Series aa = a.to(ScalarType::Float32).contiguous();
      Series out_f = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
      native::mps::unary(op, aa, out_f);
      return out_f.to(a.scalar_type());
    }
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "unary op",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          auto fn = native::cpu::lookup_unary(op, a.scalar_type());
          PLUSS_CHECK(fn != nullptr, "unary kernel missing");
          fn(a, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "unary op");
  auto fn = native::cpu::lookup_unary(op, a.scalar_type());
  PLUSS_CHECK(fn != nullptr, "unary kernel missing");
  fn(a, out);
  return out;
}

template <typename CpuUnaryFn>
Series unary_cpu_with_platform_dispatch_policy_(
    const Series& a,
    std::string_view op_name,
    CpuUnaryFn cpu_fn,
    ScalarType out_scalar_type) {
  Series out = Series::empty(a.sizes(), out_scalar_type, a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        op_name,
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          cpu_fn(a, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), op_name);
  cpu_fn(a, out);
  return out;
}

template <typename CpuUnaryFn>
Series unary_cpu_with_platform_dispatch_policy_(const Series& a, std::string_view op_name, CpuUnaryFn cpu_fn) {
  return unary_cpu_with_platform_dispatch_policy_(a, op_name, cpu_fn, a.scalar_type());
}

static bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  return a.sizes().size() == b.sizes().size() &&
         std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
}

static Series reduce_broadcasted_sensitivity_to_like_(const Series& sens, const Series& like, const char* op) {
  if (same_shape_(sens, like)) return sens;
  if (like.numel() == 1) return sens.sum();

  std::vector<std::int64_t> sens_shape(sens.sizes().begin(), sens.sizes().end());
  std::vector<std::int64_t> like_shape(like.sizes().begin(), like.sizes().end());
  if (sens_shape.size() < like_shape.size()) {
    PLUSS_FAIL(std::string(op) + ": unsupported sensitivity reduction rank mismatch");
  }

  const std::size_t prefix = sens_shape.size() - like_shape.size();
  std::vector<std::int64_t> aligned_like(sens_shape.size(), 1);
  for (std::size_t i = 0; i < like_shape.size(); ++i) {
    aligned_like[prefix + i] = like_shape[i];
  }

  Series out = sens;
  for (std::size_t i = 0; i < sens_shape.size(); ++i) {
    const auto sd = sens_shape[i];
    const auto ld = aligned_like[i];
    if (sd == ld) continue;
    if (ld != 1) {
      PLUSS_FAIL(std::string(op) + ": unsupported sensitivity reduction for non-broadcasted dims");
    }
    out = out.sum(static_cast<std::int64_t>(i), /*keepdim=*/true);
  }

  if (!same_shape_(out, like)) {
    out = out.reshape(like_shape);
  }
  return out;
}

static void maybe_link_binary_op_(
    const Series& out,
    const Series& a,
    const Series& b,
    const char* op,
    std::function<Series(const Series&)> lhs_rule,
    std::function<Series(const Series&)> rhs_rule) {
  if (!series_tape::recording_enabled()) return;
  const bool ta = series_tape::tracks_slope(a);
  const bool tb = series_tape::tracks_slope(b);
  if (!ta && !tb) return;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op;
  link->parents = {a, b};
  link->propagate = [a, b, ta, tb, op, lhs_rule = std::move(lhs_rule), rhs_rule = std::move(rhs_rule)](const Series& out_sens) {
    if (ta) {
      const Series ga = reduce_broadcasted_sensitivity_to_like_(lhs_rule(out_sens), a, op);
      series_tape::accumulate_sensitivity(a, ga);
    }
    if (tb) {
      const Series gb = reduce_broadcasted_sensitivity_to_like_(rhs_rule(out_sens), b, op);
      series_tape::accumulate_sensitivity(b, gb);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void maybe_link_unary_op_(
    const Series& out,
    const Series& a,
    const char* op,
    std::function<Series(const Series&)> rule) {
  if (!series_tape::recording_enabled()) return;
  if (!series_tape::tracks_slope(a)) return;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op ? op : "unary";
  link->parents = {a};
  link->propagate = [a, rule = std::move(rule)](const Series& out_sens) {
    series_tape::accumulate_sensitivity(a, rule(out_sens));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void maybe_link_unary_op_(
    const Series& out,
    const Series& a,
    std::function<Series(const Series&)> rule) {
  maybe_link_unary_op_(out, a, "unary", std::move(rule));
}

static void maybe_link_where_op_(const Series& out, const Series& cond, const Series& a, const Series& b) {
  if (!series_tape::recording_enabled()) return;
  const bool ta = series_tape::tracks_slope(a);
  const bool tb = series_tape::tracks_slope(b);
  if (!ta && !tb) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "where";
  link->parents = {a, b};
  link->propagate = [cond, a, b, ta, tb](const Series& out_sens) {
    const Series mask = (cond.scalar_type() == ScalarType::Bool ? cond : cond.to(ScalarType::Bool))
                            .expand(out_sens.sizes(), true);
    Series sens = out_sens;
    const Series zero = sens.mul(0.0);
    if (ta) {
      Series ga = where(mask, sens, zero);
      ga = reduce_broadcasted_sensitivity_to_like_(ga, a, "where");
      if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
      if (ga.platform() != a.platform()) ga = ga.to(a.platform());
      series_tape::accumulate_sensitivity(a, ga);
    }
    if (tb) {
      Series gb = where(mask, zero, sens);
      gb = reduce_broadcasted_sensitivity_to_like_(gb, b, "where");
      if (gb.scalar_type() != b.scalar_type()) gb = gb.to(b.scalar_type());
      if (gb.platform() != b.platform()) gb = gb.to(b.platform());
      series_tape::accumulate_sensitivity(b, gb);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void maybe_link_transport_op_(const Series& out, const Series& input, std::string op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(input)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = std::move(op_name);
  link->parents = {input};
  link->propagate = [input](const Series& out_sens) {
    Series grad = reduce_broadcasted_sensitivity_to_like_(out_sens, input, "transport");
    if (grad.scalar_type() != input.scalar_type()) grad = grad.to(input.scalar_type());
    if (grad.platform() != input.platform()) grad = grad.to(input.platform());
    series_tape::accumulate_sensitivity(input, grad);
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void maybe_link_alias_lineage_(const Series& out, const Series& input, const char* op_name) {
  if (!series_tape::recording_enabled()) return;
  if (!out.defined() || !input.defined()) return;
  if (out.storage().handle().get() != input.storage().handle().get()) return;
  if (series_tape::source_link(out)) return;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name ? op_name : "alias";
  link->parents = {input};
  // Alias lineage metadata only; propagation is attached when/if this alias
  // becomes tracked by later mutating ops (e.g. copy_ on a view).
  series_tape::bind_alias_link(out, std::move(link));
}

static Series i64_index_vector_on_(std::span<const std::int64_t> values, Platform platform) {
  const std::array<std::int64_t, 1> shape{{static_cast<std::int64_t>(values.size())}};
  Series out = Series::empty(std::span<const std::int64_t>(shape.data(), shape.size()), ScalarType::Int64, platform);
  auto* ptr = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(values.size()); ++i) {
    ptr[i] = values[static_cast<std::size_t>(i)];
  }
  return out;
}

Series add(const Series& a, const Series& b) {
  const auto out_dt = promoted_binary_dtype_(native::cpu::BinaryOpId::Add, a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Add, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "add",
      [](const Series& g) { return g; },
      [](const Series& g) { return g; });
  return out;
}

Series sub(const Series& a, const Series& b) {
  const auto out_dt = promoted_binary_dtype_(native::cpu::BinaryOpId::Sub, a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Sub, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "sub",
      [](const Series& g) { return g; },
      [](const Series& g) { return g.neg(); });
  return out;
}

static Series mul_backward_product_(const Series& lhs, const Series& rhs) {
  require_same_platform(lhs, rhs, "mul(backprop)");
  // On MPS, recurrent backprop can hit non-finite products in native multiply;
  // use a stable CPU product while tape recording is paused during reverse.
  if (lhs.platform() == Platform::MPS && !series_tape::recording_enabled()) {
    Series lhs_cpu = lhs.to(Platform::CPU, ScalarType::Float32);
    Series rhs_cpu = rhs.to(Platform::CPU, ScalarType::Float32);
    Series out_cpu = lhs_cpu.mul(rhs_cpu);
    if (lhs.scalar_type() != ScalarType::Float32) out_cpu = out_cpu.to(lhs.scalar_type());
    return out_cpu.to(lhs.platform(), lhs.scalar_type());
  }
  return lhs.mul(rhs);
}

Series mul(const Series& a, const Series& b) {
  const auto out_dt = promoted_binary_dtype_(native::cpu::BinaryOpId::Mul, a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Mul, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "mul",
      [b](const Series& g) { return mul_backward_product_(g, b); },
      [a](const Series& g) { return mul_backward_product_(g, a); });
  return out;
}
Series div(const Series& a, const Series& b) {
  const auto out_dt = promoted_binary_dtype_(native::cpu::BinaryOpId::Div, a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Div, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "div",
      [b](const Series& g) { return g.div(b); },
      [a, b](const Series& g) {
        const Series denom = b.mul(b);
        return g.mul(a).div(denom).neg();
      });
  return out;
}
Series pow(const Series& a, const Series& b) {
  const bool base_is_int = dtype_of(a.scalar_type()).is_integral();
  const bool exp_is_int = dtype_of(b.scalar_type()).is_integral();
  const bool exp_is_signed_int =
      b.scalar_type() == ScalarType::Int8 ||
      b.scalar_type() == ScalarType::Int16 ||
      b.scalar_type() == ScalarType::Int32 ||
      b.scalar_type() == ScalarType::Int64;

  const bool needs_fractional_promotion =
      base_is_int &&
      exp_is_int &&
      exp_is_signed_int &&
      lt(b, Series::full({1}, 0.0, b.scalar_type()).to(b.platform())).any().item<bool>();

  ScalarType out_dt = needs_fractional_promotion
                          ? ScalarType::Float64
                          : promoted_binary_dtype_(native::cpu::BinaryOpId::Pow, a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Pow, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "pow",
      [a, b](const Series& g) {
        return g.mul(b).mul(pow(a, b)).div(a);
      },
      [a, b](const Series& g) {
        return g.mul(pow(a, b)).mul(log(a));
      });
  return out;
}

static Series scalar_like_(const Series& a, double s) {
  const std::array<std::int64_t, 1> one{{1}};
  return Series::full(std::span<const std::int64_t>(one.data(), one.size()), s, a.scalar_type())
      .to(a.platform());
}

Series add(const Series& a, double s) { return add(a, scalar_like_(a, s)); }
Series sub(const Series& a, double s) { return sub(a, scalar_like_(a, s)); }
Series mul(const Series& a, double s) { return mul(a, scalar_like_(a, s)); }
Series div(const Series& a, double s) { return div(a, scalar_like_(a, s)); }
Series pow(const Series& a, double s) {
  const bool base_is_int = dtype_of(a.scalar_type()).is_integral();
  const bool exponent_is_integer =
      std::isfinite(s) &&
      (std::trunc(s) == s) &&
      (s >= static_cast<double>(std::numeric_limits<std::int64_t>::min())) &&
      (s <= static_cast<double>(std::numeric_limits<std::int64_t>::max()));
  if (base_is_int && exponent_is_integer && s >= 0.0) {
    return pow(a, Series::full({1}, s, ScalarType::Int64).to(a.platform()));
  }
  const ScalarType scalar_dt = (a.platform() == Platform::MPS) ? ScalarType::Float32 : ScalarType::Float64;
  return pow(a, Series::full({1}, s, scalar_dt).to(a.platform()));
}

Series& add_(Series& a, double s) { a = add(a, s); return a; }
Series& sub_(Series& a, double s) { a = sub(a, s); return a; }
Series& mul_(Series& a, double s) { a = mul(a, s); return a; }
Series& div_(Series& a, double s) { a = div(a, s); return a; }
Series& add_(Series& a, const Series& b) { a = add(a, b); return a; }
Series& sub_(Series& a, const Series& b) { a = sub(a, b); return a; }
Series& mul_(Series& a, const Series& b) { a = mul(a, b); return a; }
Series& div_(Series& a, const Series& b) { a = div(a, b); return a; }

Series addcmul(const Series& a, const Series& b, const Series& c, double value) {
  return add(a, mul(mul(b, c), value));
}
Series& addcmul_(Series& a, const Series& b, const Series& c, double value) {
  a = addcmul(a, b, c, value);
  return a;
}
Series addcdiv(const Series& a, const Series& b, const Series& c, double value) {
  return add(a, mul(div(b, c), value));
}
Series& addcdiv_(Series& a, const Series& b, const Series& c, double value) {
  a = addcdiv(a, b, c, value);
  return a;
}


Series conj(const Series& a) {
  PLUSS_CHECK(a.defined(), "conj: undefined");
  if (a.platform() == Platform::MPS &&
      (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128)) {
    require_backend_dispatch_supported_(a.platform(), "conj(complex)");
  }
  if (a.scalar_type() == ScalarType::Complex64) {
    Series out = Series::empty(a.sizes(), ScalarType::Complex64, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<Complex64*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex64*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::conj(ap[i]);
    return out;
  }
  if (a.scalar_type() == ScalarType::Complex128) {
    Series out = Series::empty(a.sizes(), ScalarType::Complex128, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<Complex128*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex128*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::conj(ap[i]);
    return out;
  }
  // For real dtypes, conj is identity.
  return a.clone();
}

Series real(const Series& a) {
  PLUSS_CHECK(a.defined(), "real: undefined");
  if (a.platform() == Platform::MPS &&
      (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128)) {
    require_backend_dispatch_supported_(a.platform(), "real(complex)");
  }
  if (a.scalar_type() == ScalarType::Complex64) {
    Series out = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex64*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::real(ap[i]);
    return out;
  }
  if (a.scalar_type() == ScalarType::Complex128) {
    Series out = Series::empty(a.sizes(), ScalarType::Float64, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex128*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::real(ap[i]);
    return out;
  }
  return a.clone();
}

Series imag(const Series& a) {
  PLUSS_CHECK(a.defined(), "imag: undefined");
  if (a.platform() == Platform::MPS &&
      (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128)) {
    require_backend_dispatch_supported_(a.platform(), "imag(complex)");
  }
  if (a.scalar_type() == ScalarType::Complex64) {
    Series out = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex64*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::imag(ap[i]);
    return out;
  }
  if (a.scalar_type() == ScalarType::Complex128) {
    Series out = Series::empty(a.sizes(), ScalarType::Float64, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex128*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::imag(ap[i]);
    return out;
  }
  // real dtype -> 0
  Series out =
      Series::zeros(a.sizes(), (a.scalar_type()==ScalarType::Float64)?ScalarType::Float64:ScalarType::Float32)
          .to(a.platform());
  return out;
}

Series angle(const Series& a) {
  PLUSS_CHECK(a.defined(), "angle: undefined");
  if (a.platform() == Platform::MPS &&
      (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128)) {
    require_backend_dispatch_supported_(a.platform(), "angle(complex)");
  }
  if (a.scalar_type() == ScalarType::Complex64) {
    Series out = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex64*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::arg(ap[i]);
    return out;
  }
  if (a.scalar_type() == ScalarType::Complex128) {
    Series out = Series::empty(a.sizes(), ScalarType::Float64, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex128*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::arg(ap[i]);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    // Use atan2(0, x) to match torch.angle(real) semantics and avoid
    // extra predicate/where plumbing on MPS.
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series zero = scalar_like_(aa, 0.0);
    return atan2(zero, aa);
  }
  // For real inputs, match torch.angle semantics: atan2(0,x)
  Series out =
      Series::empty(a.sizes(), (a.scalar_type()==ScalarType::Float64)?ScalarType::Float64:ScalarType::Float32, a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  if (out.scalar_type()==ScalarType::Float64) {
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::atan2(0.0, ap[i]);
  } else {
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* ap = reinterpret_cast<const float*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::atan2(0.0f, ap[i]);
  }
  return out;
}

Series abs(const Series& a) {
  if (a.platform() == Platform::MPS &&
      (a.scalar_type() == ScalarType::Complex64 || a.scalar_type() == ScalarType::Complex128)) {
    require_backend_dispatch_supported_(a.platform(), "abs(complex)");
  }
  if (a.scalar_type() == ScalarType::Complex64) {
    Series out = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<float*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex64*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::abs(ap[i]);
    return out;
  }
  if (a.scalar_type() == ScalarType::Complex128) {
    Series out = Series::empty(a.sizes(), ScalarType::Float64, a.platform());
    auto aa = a.contiguous();
    auto* op = reinterpret_cast<double*>(out.raw_data());
    const auto* ap = reinterpret_cast<const Complex128*>(aa.raw_data());
    for (std::int64_t i=0;i<aa.numel();++i) op[i] = std::abs(ap[i]);
    return out;
  }
  auto out = unary_op_(native::cpu::UnaryOpId::Abs, a);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "abs";
    link->parents = {a};
    link->propagate = [a](const Series& out_sens) {
      const Series zero = scalar_like_(a, 0.0);
      const Series pos = gt(a, zero).to(a.scalar_type());
      const Series neg = lt(a, zero).to(a.scalar_type());
      const Series slope = sub(pos, neg);
      series_tape::accumulate_sensitivity(a, mul(out_sens, slope));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}
Series& abs_(Series& a) { a = abs(a); return a; }
Series neg(const Series& a) {
  auto out = unary_op_(native::cpu::UnaryOpId::Neg, a);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "neg";
    link->parents = {a};
    link->propagate = [a](const Series& out_sens) {
      series_tape::accumulate_sensitivity(a, out_sens.neg());
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

Series eq(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Eq, a, b); }
Series ne(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Ne, a, b); }
Series lt(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Lt, a, b); }
Series le(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Le, a, b); }
Series gt(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Gt, a, b); }
Series ge(const Series& a, const Series& b) { return cmp_op_(native::cpu::BinaryOpId::Ge, a, b); }

Series sqrt(const Series& a) {
  auto needs_float_promotion_for_sqrt = [](ScalarType dt) noexcept {
    if (dt == ScalarType::Bool) return true;
    if (dtype_of(dt).is_integral()) return true;
    switch (dt) {
      case ScalarType::Float8E4M3FN:
      case ScalarType::Float8E5M2:
        return true;
      default:
        return false;
    }
  };

  const bool promote_input = needs_float_promotion_for_sqrt(a.scalar_type());
  const ScalarType in_dt = promote_input ? promote_types(a.scalar_type(), ScalarType::Float32) : a.scalar_type();
  const Series aa = promote_input ? a.to(in_dt) : a;

  Series out;
  if (aa.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Sqrt, aa.scalar_type());
    require_mps_support_or_policy_(supported, "sqrt");
    if (supported) {
      out = Series::empty(aa.sizes(), aa.scalar_type(), aa.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Sqrt, aa.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(aa, "sqrt", native::cpu::sqrt);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(aa, "sqrt", native::cpu::sqrt);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    return g.mul(0.5).mul(rsqrt(a));
  });
  return out;
}
Series exp(const Series& a) {
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Exp, a.scalar_type());
    require_mps_support_or_policy_(supported, "exp");
    if (supported) {
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Exp, a.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(a, "exp", native::cpu::exp);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(a, "exp", native::cpu::exp);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    return g.mul(exp(a));
  });
  return out;
}
Series log(const Series& a) {
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Log, a.scalar_type());
    require_mps_support_or_policy_(supported, "log");
    if (supported) {
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Log, a.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(a, "log", native::cpu::log);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(a, "log", native::cpu::log);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    return g.div(a);
  });
  return out;
}

Series& exp_(Series& a) {
  a = exp(a);
  return a;
}
Series& log_(Series& a) {
  a = log(a);
  return a;
}
Series& sqrt_(Series& a) {
  a = sqrt(a);
  return a;
}

Series floor(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Floor, a.scalar_type());
    require_mps_support_or_policy_(supported, "floor");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Floor, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "floor", native::cpu::floor);
}
Series ceil(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Ceil, a.scalar_type());
    require_mps_support_or_policy_(supported, "ceil");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Ceil, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "ceil", native::cpu::ceil);
}
Series round(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Round, a.scalar_type());
    require_mps_support_or_policy_(supported, "round");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Round, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "round", native::cpu::round);
}
Series trunc(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Trunc, a.scalar_type());
    require_mps_support_or_policy_(supported, "trunc");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Trunc, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "trunc", native::cpu::trunc);
}
Series isnan(const Series& a) {
  auto cpu_route = [](const Series& in, Series& out) { native::cpu::isnan(in, out); };
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
    const bool supported =
        native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsNan, a.scalar_type());
    require_mps_support_or_policy_(supported, "isnan");
    if (supported) {
      native::mps::unary_pred(native::mps::UnaryPredOp::IsNan, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "isnan", cpu_route, ScalarType::Bool);
}
Series isinf(const Series& a) {
  auto cpu_route = [](const Series& in, Series& out) { native::cpu::isinf(in, out); };
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
    const bool supported =
        native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsInf, a.scalar_type());
    require_mps_support_or_policy_(supported, "isinf");
    if (supported) {
      native::mps::unary_pred(native::mps::UnaryPredOp::IsInf, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "isinf", cpu_route, ScalarType::Bool);
}

Series sin(const Series& a) {
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Sin, a.scalar_type());
    require_mps_support_or_policy_(supported, "sin");
    if (supported) {
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Sin, a.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(a, "sin", native::cpu::sin);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(a, "sin", native::cpu::sin);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    return g.mul(cos(a));
  });
  return out;
}
Series cos(const Series& a) {
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Cos, a.scalar_type());
    require_mps_support_or_policy_(supported, "cos");
    if (supported) {
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Cos, a.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(a, "cos", native::cpu::cos);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(a, "cos", native::cpu::cos);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    return g.mul(sin(a)).neg();
  });
  return out;
}
Series tanh(const Series& a) {
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Tanh, a.scalar_type());
    require_mps_support_or_policy_(supported, "tanh");
    if (supported) {
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Tanh, a.contiguous(), out);
    } else {
      out = unary_cpu_with_platform_dispatch_policy_(a, "tanh", native::cpu::tanh);
    }
  } else {
    out = unary_cpu_with_platform_dispatch_policy_(a, "tanh", native::cpu::tanh);
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    if (a.platform() == Platform::MPS && !series_tape::recording_enabled()) {
      Series a_cpu = a.to(Platform::CPU, ScalarType::Float32);
      Series g_cpu = g.to(Platform::CPU, ScalarType::Float32);
      const Series y_cpu = tanh(a_cpu);
      const Series one_cpu = scalar_like_(a_cpu, 1.0);
      Series grad_cpu = mul(g_cpu, sub(one_cpu, mul(y_cpu, y_cpu)));
      if (a.scalar_type() != ScalarType::Float32) grad_cpu = grad_cpu.to(a.scalar_type());
      return grad_cpu.to(a.platform(), a.scalar_type());
    }
    const Series y = tanh(a);
    const Series one = scalar_like_(a, 1.0);
    return g.mul(sub(one, mul(y, y)));
  });
  return out;
}

Series tan(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Tan, a.scalar_type());
    require_mps_support_or_policy_(supported, "tan");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Tan, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "tan", native::cpu::tan);
}
Series asin(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Asin, a.scalar_type());
    require_mps_support_or_policy_(supported, "asin");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Asin, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "asin", native::cpu::asin);
}
Series acos(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Acos, a.scalar_type());
    require_mps_support_or_policy_(supported, "acos");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Acos, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "acos", native::cpu::acos);
}
Series atan(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Atan, a.scalar_type());
    require_mps_support_or_policy_(supported, "atan");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Atan, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "atan", native::cpu::atan);
}
Series sinh(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Sinh, a.scalar_type());
    require_mps_support_or_policy_(supported, "sinh");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Sinh, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "sinh", native::cpu::sinh);
}
Series cosh(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Cosh, a.scalar_type());
    require_mps_support_or_policy_(supported, "cosh");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Cosh, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "cosh", native::cpu::cosh);
}

Series log1p(const Series& a) { return unary_op_(native::cpu::UnaryOpId::Log1p, a); }
Series expm1(const Series& a) { return unary_op_(native::cpu::UnaryOpId::Expm1, a); }
Series reciprocal(const Series& a) {
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Reciprocal, a.scalar_type());
    require_mps_support_or_policy_(supported, "reciprocal");
    if (supported) {
      native::mps::unary_math(native::mps::UnaryMathOp::Reciprocal, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "reciprocal", native::cpu::reciprocal);
}

Series erf(const Series& a) {
  if (a.platform() != Platform::CPU) {
    return stable_erf_noncpu_(a);
  }
  return unary_op_(native::cpu::UnaryOpId::Erf, a);
}
Series erfc(const Series& a) {
  if (a.platform() != Platform::CPU) {
    return stable_erfc_noncpu_(a);
  }
  return unary_op_(native::cpu::UnaryOpId::Erfc, a);
}
Series lgamma(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    const auto one = scalar_like_(x, 1.0);
    const auto half = scalar_like_(x, 0.5);
    const auto c0 = scalar_like_(x, 0.9189385332046727);
    const auto invx = reciprocal(x);
    const auto invx2 = mul(invx, invx);
    const auto corr = add(
        mul(invx, scalar_like_(x, 1.0 / 12.0)),
        add(mul(mul(invx2, invx), scalar_like_(x, -1.0 / 360.0)),
            mul(mul(mul(invx2, invx2), invx), scalar_like_(x, 1.0 / 1260.0))));
    auto out = add(add(sub(mul(sub(x, half), log(x)), x), c0), corr);
    const auto nan = scalar_like_(x, std::numeric_limits<float>::quiet_NaN());
    out = where(le(x, scalar_like_(x, 0.0)), nan, out);
    return out.to(a.scalar_type());
  }
  return unary_op_(native::cpu::UnaryOpId::LGamma, a);
}
Series asinh(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    const auto one = scalar_like_(x, 1.0);
    return log(add(x, sqrt(add(mul(x, x), one)))).to(a.scalar_type());
  }
  return unary_op_(native::cpu::UnaryOpId::Asinh, a);
}
Series acosh(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    const auto one = scalar_like_(x, 1.0);
    return log(add(x, sqrt(mul(sub(x, one), add(x, one))))).to(a.scalar_type());
  }
  return unary_op_(native::cpu::UnaryOpId::Acosh, a);
}
Series atanh(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    const auto one = scalar_like_(x, 1.0);
    return mul(scalar_like_(x, 0.5), log(div(add(one, x), sub(one, x)))).to(a.scalar_type());
  }
  return unary_op_(native::cpu::UnaryOpId::Atanh, a);
}

// Shape ops
Series squeeze(const Series& a) { return a.squeeze(); }
Series squeeze(const Series& a, std::int64_t dim) { return a.squeeze(dim); }
Series& squeeze_(Series& a) { a = a.squeeze(); return a; }
Series& squeeze_(Series& a, std::int64_t dim) { a = a.squeeze(dim); return a; }

// Bitwise ops
Series bitwise_not(const Series& a) { return a.bitwise_not(); }
Series& bitwise_not_(Series& a) { return a.bitwise_not_(); }

Series bitwise_and(const Series& a, const Series& b) { return a.bitwise_and(b); }
Series& bitwise_and_(Series& a, const Series& b) { return a.bitwise_and_(b); }

Series bitwise_or(const Series& a, const Series& b) { return a.bitwise_or(b); }
Series& bitwise_or_(Series& a, const Series& b) { return a.bitwise_or_(b); }

Series bitwise_xor(const Series& a, const Series& b) { return a.bitwise_xor(b); }
Series& bitwise_xor_(Series& a, const Series& b) { return a.bitwise_xor_(b); }

Series isfinite(const Series& a) {
  auto cpu_route = [](const Series& in, Series& out) { native::cpu::isfinite(in, out); };
  if (a.platform() == Platform::MPS) {
    Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
    const bool supported =
        native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsFinite, a.scalar_type());
    require_mps_support_or_policy_(supported, "isfinite");
    if (supported) {
      native::mps::unary_pred(native::mps::UnaryPredOp::IsFinite, a.contiguous(), out);
      return out;
    }
  }
  return unary_cpu_with_platform_dispatch_policy_(a, "isfinite", cpu_route, ScalarType::Bool);
}

Series logical_not(const Series& a) {
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool, "logical_not: expects bool tensor");
  const Series af = a.to(ScalarType::Float32);
  return eq(af, scalar_like_(af, 0.0));
}

Series logical_and(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_and: undefined");
  require_same_platform(a, b, "logical_and");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_and: expects bool tensors");
  const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  const Series af = a.expand(shape, true).to(ScalarType::Float32);
  const Series bf = b.expand(shape, true).to(ScalarType::Float32);
  return gt(mul(af, bf), scalar_like_(af, 0.5));
}

Series logical_or(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_or: undefined");
  require_same_platform(a, b, "logical_or");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_or: expects bool tensors");
  const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  const Series af = a.expand(shape, true).to(ScalarType::Float32);
  const Series bf = b.expand(shape, true).to(ScalarType::Float32);
  return gt(add(af, bf), scalar_like_(af, 0.0));
}

Series logical_xor(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_xor: undefined");
  require_same_platform(a, b, "logical_xor");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_xor: expects bool tensors");
  const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  const Series af = a.expand(shape, true).to(ScalarType::Float32);
  const Series bf = b.expand(shape, true).to(ScalarType::Float32);
  return gt(abs(sub(af, bf)), scalar_like_(af, 0.5));
}

Series masked_select(const Series& a, const Series& mask) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "masked_select: mask must be bool");
  require_same_platform(a, mask, "masked_select");
  if (a.platform() == Platform::MPS) {
    const auto bshape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
    {
      auto asz = a.sizes();
      PLUSS_CHECK(asz.size() == bshape.size(), "masked_select: a must match broadcasted shape");
      PLUSS_CHECK(std::equal(asz.begin(), asz.end(), bshape.begin()), "masked_select: a must match broadcasted shape");
    }
    const std::array<std::int64_t, 1> flat_shape{{a.numel()}};
    const Series aa_flat = a.contiguous().reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    const Series mm_flat = mask.expand(std::span<const std::int64_t>(bshape.data(), bshape.size()), /*implicit=*/true)
                               .contiguous()
                               .reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    const Series nz2d = nonzero(mm_flat);
    if (nz2d.numel() == 0) {
      return Series::empty({0}, a.scalar_type(), a.platform());
    }
    const Series idx = nz2d.to(Platform::CPU).squeeze(-1).contiguous().to(Platform::MPS);
    return gather(aa_flat, 0, idx);
  }
  auto cpu_route = [&]() -> Series {
    // Broadcast mask to a's shape (Torch-like).
    auto bshape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
    {
      auto asz = a.sizes();
      PLUSS_CHECK(asz.size() == bshape.size(), "masked_select: a must match broadcasted shape");
      PLUSS_CHECK(std::equal(asz.begin(), asz.end(), bshape.begin()), "masked_select: a must match broadcasted shape");
    }
    auto mm = mask.expand(std::span<const std::int64_t>(bshape.data(), bshape.size()), /*implicit=*/true).contiguous();
    auto aa = a.contiguous();

    const bool* mp = reinterpret_cast<const bool*>(mm.raw_data());
    const auto n = aa.numel();
    std::int64_t count = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      if (mp[i]) ++count;
    }

    const std::array<std::int64_t, 1> out_sizes{count};
    Series out =
        Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), aa.scalar_type(), a.platform());
    const auto item = aa.dtype().itemsize;
    const std::byte* ap = aa.raw_data();
    auto* op = reinterpret_cast<std::byte*>(out.raw_mutable_data());
    std::int64_t j = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      if (!mp[i]) continue;
      std::memcpy(op + j * item, ap + i * item, (std::size_t)item);
      ++j;
    }
    return out;
  };

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_masking_enabled();
    const bool native_strict = native::cuda::native_masking_strict();
    const bool route_supported = native::cuda::supports_masked_select(a, mask);
    return cuda_masking_route_or_bridge_(
        "masked_select",
        native_enabled,
        native_strict,
        route_supported,
        cpu_route);
  }

  require_backend_dispatch_supported_(a.platform(), "masked_select");
  return cpu_route();
}


Series masked_fill(const Series& a, const Series& mask, const Series& value) {
  Series out = a.clone();
  masked_fill_(out, mask, value);
  return out;
}

Series masked_fill(const Series& a, const Series& mask, double value) {
  Series out = a.clone();
  masked_fill_(out, mask, value);
  return out;
}

Series& masked_fill_(Series& a, const Series& mask, const Series& value) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "masked_fill_: mask must be bool");
  PLUSS_CHECK(value.numel() == 1, "masked_fill_: value must be scalar");
  require_same_platform(a, mask, "masked_fill_");
  require_same_platform(a, value, "masked_fill_");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
  // If shapes differ, expand a to shape then write back only if same storage: for now require same shape to keep safe alias invariants.
  {
    auto asz = a.sizes();
    PLUSS_CHECK(asz.size() == shape.size(), "masked_fill_: shape mismatch (in-place requires mask broadcast to match a)");
    PLUSS_CHECK(std::equal(asz.begin(), asz.end(), shape.begin()), "masked_fill_: shape mismatch (in-place requires mask broadcast to match a)");
  }
  if (a.platform() == Platform::MPS) {
    const std::array<std::int64_t, 1> flat_shape{{a.numel()}};
    const Series mm_flat =
        mask.expand(shape, true)
            .contiguous()
            .reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    const Series nz2d = nonzero(mm_flat);
    const std::int64_t count = nz2d.numel();
    if (count == 0) {
      return a;
    }
    const Series idx = nz2d.to(Platform::CPU).squeeze(-1).contiguous().to(Platform::MPS);
    const std::array<std::int64_t, 1> src_shape{{count}};
    const std::array<std::int64_t, 1> one_shape{{1}};
    const Series scalar_v =
        value.to(a.scalar_type())
            .contiguous()
            .reshape(std::span<const std::int64_t>(one_shape.data(), one_shape.size()));
    const Series ss = scalar_v
                          .expand(std::span<const std::int64_t>(src_shape.data(), src_shape.size()), true)
                          .contiguous();
    Series out = a.contiguous();
    Series out_flat = out.reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    out_flat = scatter(out_flat, 0, idx, ss);
    (void)static_cast<const Series&>(out_flat).raw_data();
    out = out_flat.reshape(shape);
    if (a.is_contiguous()) {
      a = out;
    } else {
      a.copy_(out);
    }
    return a;
  }
  auto cpu_apply = [&](Series& target) -> Series& {
    const Series mm = mask.expand(shape, true);
    Series vv = value.to(target.scalar_type()).contiguous();
    const auto item = target.dtype().itemsize;
    const std::byte* vp = vv.raw_data();
    auto* ap = reinterpret_cast<std::byte*>(target.raw_mutable_data());

    if (target.is_contiguous() && mm.is_contiguous()) {
      const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
      const auto n = target.numel();
      for (std::int64_t i = 0; i < n; ++i) {
        if (!mp[i]) continue;
        std::memcpy(ap + i * item, vp, (std::size_t)item);
      }
      return target;
    }

    const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
    SeriesIterator it = SeriesIterator::unary(mm, target);
    it.for_each_index([&](std::int64_t, std::int64_t m_off, std::int64_t, std::int64_t out_off) {
      if (!mp[m_off]) return;
      std::memcpy(
          ap + static_cast<std::ptrdiff_t>(out_off) * static_cast<std::ptrdiff_t>(item),
          vp,
          static_cast<std::size_t>(item));
    });
    return target;
  };

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_masking_enabled();
    const bool native_strict = native::cuda::native_masking_strict();
    const bool route_supported = native::cuda::supports_masked_fill(a, mask, value);
    a = cuda_masking_route_or_bridge_(
        "masked_fill_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = a.clone();
          cpu_apply(out);
          return out;
        });
    return a;
  }

  require_backend_dispatch_supported_(a.platform(), "masked_fill_");
  cpu_apply(a);
  return a;
}

Series& masked_fill_(Series& a, const Series& mask, double value) {
  const std::array<std::int64_t, 1> one{1};
  auto v = Series::full(std::span<const std::int64_t>(one.data(), one.size()), value, a.scalar_type())
               .to(a.platform());
  return masked_fill_(a, mask, v);
}

static std::vector<std::int64_t> out_sizes_for_reduce_(std::span<const std::int64_t> sizes, std::int64_t dim, bool keepdim) {
  std::vector<std::int64_t> out(sizes.begin(), sizes.end());
  if (dim < 0) dim += (std::int64_t)out.size();
  PLUSS_CHECK(dim >= 0 && dim < (std::int64_t)out.size(), "reduce: dim out of range");
  if (keepdim) out[(std::size_t)dim] = 1;
  else out.erase(out.begin() + dim);
  return out;
}

static Series maybe_keepdim_(Series out, std::span<const std::int64_t> in_sizes, std::int64_t dim, bool keepdim) {
  if (!keepdim) return out;
  const auto nd = static_cast<std::int64_t>(in_sizes.size());
  if (nd == 0) return out;
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "reduce: dim out of range");
  std::vector<std::int64_t> ks(in_sizes.begin(), in_sizes.end());
  ks[(std::size_t)dim] = 1;
  return out.reshape(ks);
}

static Series reverse_dim_(const Series& x, std::int64_t dim) {
  const auto nd = x.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "reverse_dim: dim out of range");
  const std::int64_t n = x.sizes()[static_cast<std::size_t>(dim)];
  Series idx = Series::arange(static_cast<double>(n - 1), -1.0, -1.0, ScalarType::Int64).to(x.platform());
  std::vector<std::int64_t> idx_shape(static_cast<std::size_t>(nd), 1);
  idx_shape[static_cast<std::size_t>(dim)] = n;
  const Series expanded = idx.reshape(idx_shape).expand(x.sizes(), true).contiguous();
  return gather(x, dim, expanded);
}

static void maybe_link_extrema_reduce_(
    const Series& out,
    const Series& input,
    const Series& reduced,
    std::int64_t dim,
    bool keepdim,
    const char* op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(input)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name;
  link->parents = {input};
  link->propagate = [input, reduced, dim, keepdim](const Series& out_sens) {
    Series value = reduced;
    Series sens = out_sens;
    if (!keepdim) {
      value = value.unsqueeze(dim);
      sens = sens.unsqueeze(dim);
    }
    value = value.expand(input.sizes(), true);
    sens = sens.expand(input.sizes(), true);
    if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
    if (sens.platform() != input.platform()) sens = sens.to(input.platform());

    const Series mask = eq(input, value);
    const Series mask_value = mask.to(input.scalar_type());
    const Series count = sum(mask_value, dim, /*keepdim=*/true).expand(input.sizes(), true);
    const Series grad = mask_value.mul(sens).div(count);
    series_tape::accumulate_sensitivity(input, grad);
  };
  series_tape::bind_source_link(out, std::move(link));
}

Series sum(const Series& a) {
  PLUSS_CHECK(a.defined(), "sum: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  auto aa = a.contiguous();
  if (aa.scalar_type() == ScalarType::Bool) {
    // Bool sum is a count reduction.
    aa = aa.to(ScalarType::Int64);
  }
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      Series cur = aa;
      for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
        cur = sum(cur, d, false);
      }
      if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
        auto link = std::make_shared<series_tape::SourceLink>();
        link->op_name = "sum";
        link->parents = {a};
        link->propagate = [a](const Series& out_sens) {
          const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(out_sens);
          series_tape::accumulate_sensitivity(a, ga);
        };
        series_tape::bind_source_link(cur, std::move(link));
      }
      return cur;
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_sum_reduction(aa);
      (void)cuda_reduction_route_or_bridge_(
          "sum",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "sum");
    }
  }
  Series out = Series::empty({}, aa.scalar_type(), aa.platform());
  native::cpu::sum_all(aa, out);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "sum";
    link->parents = {a};
    link->propagate = [a](const Series& out_sens) {
      const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(out_sens);
      series_tape::accumulate_sensitivity(a, ga);
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

Series sum(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "sum: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  auto aa = a.contiguous();
  if (aa.scalar_type() == ScalarType::Bool) {
    // Bool sum is a count reduction.
    aa = aa.to(ScalarType::Int64);
  }
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, /*keepdim=*/false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      if (native::mps::supports_sum_dim(aa.scalar_type())) {
        native::mps::sum_dim(aa, dim, out);
        auto reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
        if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
          const auto norm_dim = dim;
          auto link = std::make_shared<series_tape::SourceLink>();
          link->op_name = "sum(dim)";
          link->parents = {a};
          link->propagate = [a, norm_dim, keepdim](const Series& out_sens) {
            Series shaped = out_sens;
            if (!keepdim) {
              shaped = shaped.unsqueeze(norm_dim);
            }
            shaped = shaped.expand(a.sizes(), true);
            const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(shaped);
            series_tape::accumulate_sensitivity(a, ga);
          };
          series_tape::bind_source_link(reduced, std::move(link));
        }
        return reduced;
      }
      const bool supports_f32 = native::mps::supports_sum_dim(ScalarType::Float32);
      require_mps_support_or_policy_(supports_f32, "sum(dim)");
      if (supports_f32) {
        const Series aa_f = aa.to(ScalarType::Float32).contiguous();
        Series out_f = Series::empty(out_sizes, ScalarType::Float32, aa.platform());
        native::mps::sum_dim(aa_f, dim, out_f);
        out = out_f.to(aa.scalar_type());
      } else {
        native::cpu::sum_dim(aa, dim, out);
      }
      auto reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
      if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
        const auto norm_dim = dim;
        auto link = std::make_shared<series_tape::SourceLink>();
        link->op_name = "sum(dim)";
        link->parents = {a};
        link->propagate = [a, norm_dim, keepdim](const Series& out_sens) {
          Series shaped = out_sens;
          if (!keepdim) {
            shaped = shaped.unsqueeze(norm_dim);
          }
          shaped = shaped.expand(a.sizes(), true);
          const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(shaped);
          series_tape::accumulate_sensitivity(a, ga);
        };
        series_tape::bind_source_link(reduced, std::move(link));
      }
      return reduced;
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_sum_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "sum(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "sum(dim)");
    }
  }
  native::cpu::sum_dim(aa, dim, out);
  auto reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    const auto norm_dim = dim;
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "sum(dim)";
    link->parents = {a};
    link->propagate = [a, norm_dim, keepdim](const Series& out_sens) {
      Series shaped = out_sens;
      if (!keepdim) {
        shaped = shaped.unsqueeze(norm_dim);
      }
      shaped = shaped.expand(a.sizes(), true);
      const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(shaped);
      series_tape::accumulate_sensitivity(a, ga);
    };
    series_tape::bind_source_link(reduced, std::move(link));
  }
  return reduced;
}

Series prod(const Series& a) {
  PLUSS_CHECK(a.defined(), "prod: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  Series cur = a.contiguous();
  if (cur.platform() != Platform::CPU) {
    if (cur.platform() == Platform::MPS) {
      for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
        cur = prod(cur, d, false);
      }
      return cur;
    }
    if (cur.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_prod_reduction(cur);
      (void)cuda_reduction_route_or_bridge_(
          "prod",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return cur; });
    } else {
      require_backend_dispatch_supported_(cur.platform(), "prod");
    }
  }
  // prod_all CPU path: reduce sequentially using dim kernel.
  for (std::int64_t d = (std::int64_t)cur.dim()-1; d>=0; --d) cur = prod(cur, d, false);
  return cur;
}

Series prod(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "prod: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      if (native::mps::supports_prod_dim(aa.scalar_type())) {
        native::mps::prod_dim(aa, dim, out);
        return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
      }
      const bool supports_f32 = native::mps::supports_prod_dim(ScalarType::Float32);
      require_mps_support_or_policy_(supports_f32, "prod(dim)");
      if (supports_f32) {
        const Series aa_f = aa.to(ScalarType::Float32).contiguous();
        Series out_f = Series::empty(out_sizes, ScalarType::Float32, aa.platform());
        native::mps::prod_dim(aa_f, dim, out_f);
        out = out_f.to(aa.scalar_type());
      } else {
        native::cpu::prod_dim(aa, dim, out);
      }
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_prod_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "prod(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "prod(dim)");
    }
  }
  native::cpu::prod_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series mean(const Series& a) {
  PLUSS_CHECK(a.defined(), "mean: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  auto aa = a.contiguous();
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      Series s = sum(aa);
      const auto st = aa.scalar_type();
      if (st == ScalarType::Bool || dtype_of(st).is_integral()) {
        s = s.to(ScalarType::Float32);
      }
      return div(s, scalar_like_(s, static_cast<double>(aa.numel())));
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_mean_reduction(aa);
      (void)cuda_reduction_route_or_bridge_(
          "mean",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "mean");
    }
  }
  // mean_all kernel exists for float32/64/16/bf16 in Patch40 expand; for others, promote to float32.
  ScalarType dt = aa.scalar_type();
  if (dt != ScalarType::Float64 && dt != ScalarType::Float32 && dt != ScalarType::Float16 && dt != ScalarType::BFloat16 && dt != ScalarType::Complex64 && dt != ScalarType::Complex128) {
    aa = aa.to(ScalarType::Float32);
    dt = ScalarType::Float32;
  }
  Series out = Series::empty({}, dt, aa.platform());
  native::cpu::mean_all(aa, out);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    const double inv_n = 1.0 / static_cast<double>(a.numel());
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "mean";
    link->parents = {a};
    link->propagate = [a, inv_n](const Series& out_sens) {
      const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(out_sens).mul(inv_n);
      series_tape::accumulate_sensitivity(a, ga);
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

Series mean(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "mean: undefined");
  if (a.dim() == 0) {
    Series out = a.clone();
    maybe_link_unary_op_(out, a, [](const Series& g) { return g; });
    return out;
  }
  // mean = sum / reduce_n, for float types; for integral, promote to float32
  Series aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      Series s = sum(aa, dim, keepdim);
      const auto st = aa.scalar_type();
      if (st == ScalarType::Bool || dtype_of(st).is_integral()) {
        s = s.to(ScalarType::Float32);
      }
      return div(s, scalar_like_(s, static_cast<double>(reduce_n)));
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_mean_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "mean(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "mean(dim)");
    }
  }

  ScalarType dt = aa.scalar_type();
  if (dt != ScalarType::Float64 && dt != ScalarType::Float32 && dt != ScalarType::Float16 && dt != ScalarType::BFloat16 && dt != ScalarType::Complex64 && dt != ScalarType::Complex128) {
    aa = aa.to(ScalarType::Float32);
    dt = ScalarType::Float32;
  }
  Series s = sum(aa, dim, keepdim);
  if (dt == ScalarType::Float64) {
    auto* p = reinterpret_cast<double*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (double)reduce_n;
  } else if (dt == ScalarType::Float32) {
    auto* p = reinterpret_cast<float*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] /= (float)reduce_n;
  } else if (dt == ScalarType::Float16) {
    auto* p = reinterpret_cast<Float16*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] = float16_from_float(float16_to_float(p[i]) / (float)reduce_n);
  } else if (dt == ScalarType::BFloat16) {
    auto* p = reinterpret_cast<BFloat16*>(s.raw_data());
    for (std::int64_t i=0;i<s.numel();++i) p[i] = bfloat16_from_float(bfloat16_to_float(p[i]) / (float)reduce_n);
  } else if (dt == ScalarType::Complex64) {
    auto* p = reinterpret_cast<Complex64*>(s.raw_data());
    const float inv = 1.0f / (float)reduce_n;
    for (std::int64_t i=0;i<s.numel();++i) p[i] = p[i] * inv;
  } else if (dt == ScalarType::Complex128) {
    auto* p = reinterpret_cast<Complex128*>(s.raw_data());
    const double inv = 1.0 / (double)reduce_n;
    for (std::int64_t i=0;i<s.numel();++i) p[i] = p[i] * inv;
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    const auto norm_dim = dim;
    const double inv_n = 1.0 / static_cast<double>(reduce_n);
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "mean(dim)";
    link->parents = {a};
    link->propagate = [a, norm_dim, keepdim, inv_n](const Series& out_sens) {
      Series shaped = out_sens;
      if (!keepdim) {
        shaped = shaped.unsqueeze(norm_dim);
      }
      shaped = shaped.expand(a.sizes(), true);
      const Series ga = Series::ones(a.sizes(), a.scalar_type()).to(a.platform()).mul(shaped).mul(inv_n);
      series_tape::accumulate_sensitivity(a, ga);
    };
    series_tape::bind_source_link(s, std::move(link));
  }
  return s;
}

Series amax(const Series& a) {
  PLUSS_CHECK(a.defined(), "amax: undefined");
  if (a.platform() != Platform::CPU) {
    if (a.platform() == Platform::MPS) {
      // handled by per-dim path below
    } else if (a.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_ordered_extrema_reduction(a);
      (void)cuda_reduction_route_or_bridge_(
          "amax",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return a; });
    } else {
      require_backend_dispatch_supported_(a.platform(), "amax");
    }
  }
  if (a.dim() == 0) return a.clone();
  Series cur = a.contiguous();
  for (std::int64_t d = (std::int64_t)cur.dim()-1; d>=0; --d) cur = amax(cur, d, false);
  return cur;
}

Series amax(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "amax: complex dtype not supported for ordered reduction");

  PLUSS_CHECK(a.defined(), "amax: undefined");
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      if (native::mps::supports_amax_amin_dim(aa.scalar_type())) {
        native::mps::amax_dim(aa, dim, out);
        Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
        maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amax(dim)");
        return reduced;
      }
      const bool supports_f32 = native::mps::supports_amax_amin_dim(ScalarType::Float32);
      require_mps_support_or_policy_(supports_f32, "amax(dim)");
      if (supports_f32) {
        const Series aa_f = aa.to(ScalarType::Float32).contiguous();
        Series out_f = Series::empty(out_sizes, ScalarType::Float32, aa.platform());
        native::mps::amax_dim(aa_f, dim, out_f);
        out = out_f.to(aa.scalar_type());
      } else {
        native::cpu::amax_dim(aa, dim, out);
      }
      Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
      maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amax(dim)");
      return reduced;
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_ordered_extrema_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "amax(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "amax(dim)");
    }
  }
  native::cpu::amax_dim(aa, dim, out);
  Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amax(dim)");
  return reduced;
}

Series amin(const Series& a) {
  PLUSS_CHECK(a.defined(), "amin: undefined");
  if (a.platform() != Platform::CPU) {
    if (a.platform() == Platform::MPS) {
      // handled by per-dim path below
    } else if (a.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_ordered_extrema_reduction(a);
      (void)cuda_reduction_route_or_bridge_(
          "amin",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return a; });
    } else {
      require_backend_dispatch_supported_(a.platform(), "amin");
    }
  }
  if (a.dim() == 0) return a.clone();
  Series cur = a.contiguous();
  for (std::int64_t d = (std::int64_t)cur.dim()-1; d>=0; --d) cur = amin(cur, d, false);
  return cur;
}
Series amin(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "amin: complex dtype not supported for ordered reduction");

  PLUSS_CHECK(a.defined(), "amin: undefined");
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS) {
      if (native::mps::supports_amax_amin_dim(aa.scalar_type())) {
        native::mps::amin_dim(aa, dim, out);
        Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
        maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amin(dim)");
        return reduced;
      }
      const bool supports_f32 = native::mps::supports_amax_amin_dim(ScalarType::Float32);
      require_mps_support_or_policy_(supports_f32, "amin(dim)");
      if (supports_f32) {
        const Series aa_f = aa.to(ScalarType::Float32).contiguous();
        Series out_f = Series::empty(out_sizes, ScalarType::Float32, aa.platform());
        native::mps::amin_dim(aa_f, dim, out_f);
        out = out_f.to(aa.scalar_type());
      } else {
        native::cpu::amin_dim(aa, dim, out);
      }
      Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
      maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amin(dim)");
      return reduced;
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_ordered_extrema_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "amin(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "amin(dim)");
    }
  }
  native::cpu::amin_dim(aa, dim, out);
  Series reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  maybe_link_extrema_reduce_(reduced, a, reduced, dim, keepdim, "amin(dim)");
  return reduced;
}

Series all(const Series& a) {
  PLUSS_CHECK(a.defined(), "all: undefined");
  if (a.dim()==0) return a.to(ScalarType::Bool);
  Series cur = a.to(ScalarType::Bool).contiguous();
  for (std::int64_t d=(std::int64_t)cur.dim()-1; d>=0; --d) cur = all(cur, d, false);
  return cur;
}
Series all(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "all: undefined");
  if (a.dim()==0) return a.to(ScalarType::Bool);
  auto aa = a.to(ScalarType::Bool).contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, ScalarType::Bool, aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_all_any_dim(aa.scalar_type())) {
      native::mps::all_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_logical_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "all(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "all(dim)");
    }
  }
  native::cpu::all_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series any(const Series& a) {
  PLUSS_CHECK(a.defined(), "any: undefined");
  if (a.dim()==0) return a.to(ScalarType::Bool);
  Series cur = a.to(ScalarType::Bool).contiguous();
  for (std::int64_t d=(std::int64_t)cur.dim()-1; d>=0; --d) cur = any(cur, d, false);
  return cur;
}
Series any(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "any: undefined");
  if (a.dim()==0) return a.to(ScalarType::Bool);
  auto aa = a.to(ScalarType::Bool).contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, ScalarType::Bool, aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_all_any_dim(aa.scalar_type())) {
      native::mps::any_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    if (aa.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_reduction_enabled();
      const bool native_strict = native::cuda::native_reduction_strict();
      const bool route_supported = native::cuda::supports_logical_reduction(aa, dim);
      (void)cuda_reduction_route_or_bridge_(
          "any(dim)",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return aa; });
    } else {
      require_backend_dispatch_supported_(aa.platform(), "any(dim)");
    }
  }
  native::cpu::any_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "argmax: undefined");
  if (a.platform() == Platform::MPS) {
    if (a.dim() == 0) {
      return Series::full({}, 0.0, ScalarType::Int64).to(a.platform());
    }
    auto aa = a.contiguous();
    const auto nd = aa.dim();
    if (dim < 0) dim += nd;
    auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
    Series out = Series::empty(out_sizes, ScalarType::Int64, aa.platform());
    if (native::mps::supports_argmax_argmin_dim(aa.scalar_type())) {
      native::mps::argmax_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    const bool supports_f32 = native::mps::supports_argmax_argmin_dim(ScalarType::Float32);
    require_mps_support_or_policy_(supports_f32, "argmax");
    if (supports_f32) {
      native::mps::argmax_dim(aa.to(ScalarType::Float32).contiguous(), dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    return native::cpu::argmax(a, dim, keepdim);
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_reduction_enabled();
    const bool native_strict = native::cuda::native_reduction_strict();
    const bool route_supported = native::cuda::supports_arg_extrema_reduction(a, dim);
    (void)cuda_reduction_route_or_bridge_(
        "argmax",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return a; });
  } else {
    require_backend_dispatch_supported_(a.platform(), "argmax");
  }
  return native::cpu::argmax(a, dim, keepdim);
}
Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "argmin: undefined");
  if (a.platform() == Platform::MPS) {
    if (a.dim() == 0) {
      return Series::full({}, 0.0, ScalarType::Int64).to(a.platform());
    }
    auto aa = a.contiguous();
    const auto nd = aa.dim();
    if (dim < 0) dim += nd;
    auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
    Series out = Series::empty(out_sizes, ScalarType::Int64, aa.platform());
    if (native::mps::supports_argmax_argmin_dim(aa.scalar_type())) {
      native::mps::argmin_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    const bool supports_f32 = native::mps::supports_argmax_argmin_dim(ScalarType::Float32);
    require_mps_support_or_policy_(supports_f32, "argmin");
    if (supports_f32) {
      native::mps::argmin_dim(aa.to(ScalarType::Float32).contiguous(), dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    return native::cpu::argmin(a, dim, keepdim);
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_reduction_enabled();
    const bool native_strict = native::cuda::native_reduction_strict();
    const bool route_supported = native::cuda::supports_arg_extrema_reduction(a, dim);
    (void)cuda_reduction_route_or_bridge_(
        "argmin",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return a; });
  } else {
    require_backend_dispatch_supported_(a.platform(), "argmin");
  }
  return native::cpu::argmin(a, dim, keepdim);
}

Series where(const Series& cond, const Series& a, const Series& b) {
  PLUSS_CHECK(cond.defined() && a.defined() && b.defined(), "where: undefined");
  require_same_platform(a, b, "where");
  require_same_platform(a, cond, "where");
  auto shape = SeriesIterator::broadcast_shape(cond.sizes(), SeriesIterator::broadcast_shape(a.sizes(), b.sizes()));
  auto cc = cond;
  auto aa = a;
  auto bb = b;
  ScalarType dt = promote_types(aa.scalar_type(), bb.scalar_type());
  cc = (cc.scalar_type() == ScalarType::Bool) ? cc : cc.to(ScalarType::Bool);
  aa = aa.to(dt);
  bb = bb.to(dt);
  Series out = Series::empty(shape, dt, a.platform());
  if (a.platform() != Platform::CPU) {
    if (a.platform() == Platform::MPS && native::mps::supports_where(cc.scalar_type(), dt)) {
      native::mps::where(cc.contiguous(), aa.contiguous(), bb.contiguous(), out);
      maybe_link_where_op_(out, cond, a, b);
      return out;
    }
  }
  const auto cc_exp = cc.expand(shape, true);
  const auto aa_exp = aa.expand(shape, true);
  const auto bb_exp = bb.expand(shape, true);
  if (dt == ScalarType::Bool) {
    const Series t = logical_and(cc_exp, aa_exp);
    const Series f = logical_and(logical_not(cc_exp), bb_exp);
    return logical_or(t, f);
  }
  const Series mask = cc_exp.to(dt);
  const Series one = scalar_like_(mask, 1.0);
  return add(mul(mask, aa_exp), mul(sub(one, mask), bb_exp));
}

Series clamp(const Series& a, double min, double max) {
  Series out = a.clone();
  clamp_(out, min, max);
  maybe_link_unary_op_(out, a, [a, min, max](const Series& g) {
    const Series min_s = scalar_like_(a, min);
    const Series max_s = scalar_like_(a, max);
    const Series one = scalar_like_(a, 1.0);
    const Series zero = scalar_like_(a, 0.0);
    const Series mask = logical_and(ge(a, min_s), le(a, max_s));
    return mul(g, where(mask, one, zero));
  });
  return out;
}

Series& clamp_(Series& a, double min, double max) {
  PLUSS_CHECK(a.defined(), "clamp_: undefined");
  PLUSS_CHECK(min <= max, "clamp_: min must be <= max");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "clamp_: complex dtype not supported");
  if (a.platform() == Platform::MPS) {
    a = clamp_max(clamp_min(a, min), max);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_");
  auto aa = a.contiguous();
  native::cpu::clamp_inplace(aa, min, max);
  if (a.is_contiguous()) {
    std::memcpy(a.raw_mutable_data(), aa.raw_data(), (std::size_t)(aa.dtype().itemsize * aa.numel()));
    return a;
  }
  PLUSS_FAIL("clamp_: non-contiguous target not supported yet");
}

Series relu(const Series& a) {
  PLUSS_CHECK(a.defined(), "relu: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "relu: complex dtype not supported");
  if (a.platform() == Platform::MPS) {
    const bool supported =
        native::mps::supports_unary_math(native::mps::UnaryMathOp::Relu, a.scalar_type());
    require_mps_support_or_policy_(supported, "relu");
    if (supported) {
      Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      native::mps::unary_math(native::mps::UnaryMathOp::Relu, a.contiguous(), out);
      maybe_link_unary_op_(out, a, [a](const Series& g) {
        const Series zero = scalar_like_(a, 0.0);
        const Series one = scalar_like_(a, 1.0);
        return mul(g, where(gt(a, zero), one, zero));
      });
      return out;
    }
  }
  Series out = a.clone();
  relu_(out);
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    const Series zero = scalar_like_(a, 0.0);
    const Series one = scalar_like_(a, 1.0);
    return mul(g, where(gt(a, zero), one, zero));
  });
  return out;
}

Series& relu_(Series& a) {
  PLUSS_CHECK(a.defined(), "relu_: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "relu_: complex dtype not supported");
  return clamp_min_(a, 0.0);
}

Series sigmoid(const Series& a) {
  const Series one = scalar_like_(a, 1.0);
  Series out = div(one, add(one, exp(neg(a))));
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    if (a.platform() == Platform::MPS && !series_tape::recording_enabled()) {
      Series a_cpu = a.to(Platform::CPU, ScalarType::Float32);
      Series g_cpu = g.to(Platform::CPU, ScalarType::Float32);
      const Series y_cpu = sigmoid(a_cpu);
      const Series one_cpu = scalar_like_(a_cpu, 1.0);
      Series grad_cpu = mul(g_cpu, mul(y_cpu, sub(one_cpu, y_cpu)));
      if (a.scalar_type() != ScalarType::Float32) grad_cpu = grad_cpu.to(a.scalar_type());
      return grad_cpu.to(a.platform(), a.scalar_type());
    }
    const Series y = sigmoid(a);
    const Series one = scalar_like_(a, 1.0);
    return mul(g, mul(y, sub(one, y)));
  });
  return out;
}
Series& sigmoid_(Series& a) {
  a = sigmoid(a);
  return a;
}

Series softplus(const Series& a, double beta, double threshold) {
  const Series beta_s = scalar_like_(a, beta);
  const Series thresh_s = scalar_like_(a, threshold);
  const Series bz = mul(a, beta_s);
  const Series use_linear = gt(bz, thresh_s);
  const Series smooth = div(log1p(exp(bz)), beta_s);
  Series out = where(use_linear, a, smooth);
  maybe_link_unary_op_(out, a, [a, beta, threshold](const Series& g) {
    const Series beta_s = scalar_like_(a, beta);
    const Series thresh_s = scalar_like_(a, threshold);
    const Series one = scalar_like_(a, 1.0);
    const Series bz = mul(a, beta_s);
    const Series use_linear = gt(bz, thresh_s);
    const Series smooth_grad = sigmoid(bz);
    return mul(g, where(use_linear, one, smooth_grad));
  });
  return out;
}
Series& softplus_(Series& a, double beta, double threshold) {
  a = softplus(a, beta, threshold);
  return a;
}

Series silu(const Series& a) {
  return mul(a, sigmoid(a));
}
Series& silu_(Series& a) {
  a = silu(a);
  return a;
}

Series gelu(const Series& a, bool approximate) {
  if (approximate) {
    const Series half = scalar_like_(a, 0.5);
    const Series one = scalar_like_(a, 1.0);
    const Series c0 = scalar_like_(a, 0.7978845608028654); // sqrt(2/pi)
    const Series c1 = scalar_like_(a, 0.044715);
    const Series x2 = mul(a, a);
    const Series x3 = mul(x2, a);
    const Series inner = mul(c0, add(a, mul(c1, x3)));
    return mul(mul(half, a), add(one, tanh(inner)));
  }
  const Series half = scalar_like_(a, 0.5);
  const Series one = scalar_like_(a, 1.0);
  const Series inv_sqrt2 = scalar_like_(a, 0.7071067811865475);
  return mul(mul(half, a), add(one, erf(mul(a, inv_sqrt2))));
}
Series& gelu_(Series& a, bool approximate) {
  a = gelu(a, approximate);
  return a;
}

Series leaky_relu(const Series& a, double negative_slope) {
  const Series zero = scalar_like_(a, 0.0);
  const Series slope = scalar_like_(a, negative_slope);
  return where(ge(a, zero), a, mul(a, slope));
}
Series& leaky_relu_(Series& a, double negative_slope) {
  a = leaky_relu(a, negative_slope);
  return a;
}

Series softmax(const Series& a, std::int64_t dim) {
  const auto nd = a.dim();
  if (dim < 0) dim += nd;
  const Series aa = a.contiguous();
  const Series m = amax(aa, dim, true);
  const Series z = sub(aa, m);
  const Series e = exp(z);
  const Series s = sum(e, dim, true);
  return div(e, s);
}
Series log_softmax(const Series& a, std::int64_t dim) {
  const auto nd = a.dim();
  if (dim < 0) dim += nd;
  const Series aa = a.contiguous();
  const Series m = amax(aa, dim, true);
  const Series z = sub(aa, m);
  const Series lse = log(sum(exp(z), dim, true));
  return sub(z, lse);
}
Series logsumexp(const Series& a, std::int64_t dim, bool keepdim) {
  const auto nd = a.dim();
  if (dim < 0) dim += nd;
  const Series aa = a.contiguous();
  const Series m = amax(aa, dim, true);
  Series out = add(log(sum(exp(sub(aa, m)), dim, true)), m);
  if (!keepdim) out = out.squeeze(dim);
  return out;
}

static bool mps_scatter_has_duplicate_targets_(
    const Series& out_contig,
    std::int64_t dim,
    const Series& index_i64_contig) {
  PLUSS_CHECK(out_contig.is_contiguous(), "scatter duplicate-check: expected contiguous output");
  PLUSS_CHECK(index_i64_contig.scalar_type() == ScalarType::Int64, "scatter duplicate-check: expected int64 index");
  if (index_i64_contig.numel() <= 1) {
    return false;
  }

  const auto out_sizes = out_contig.sizes();
  const auto out_strides = out_contig.strides();
  const auto idx_sizes = index_i64_contig.sizes();
  const auto nd = static_cast<std::int64_t>(out_sizes.size());
  if (dim < 0) {
    dim += nd;
  }
  PLUSS_CHECK(dim >= 0 && dim < nd, "scatter duplicate-check: dim out of range");
  PLUSS_CHECK(
      idx_sizes.size() == out_sizes.size(),
      "scatter duplicate-check: rank mismatch between index and output");

  const Series idx_cpu = index_i64_contig.to(Platform::CPU).contiguous();
  const auto* ip = reinterpret_cast<const std::int64_t*>(idx_cpu.raw_data());
  const auto n = idx_cpu.numel();

  std::vector<std::int64_t> offsets;
  offsets.reserve(static_cast<std::size_t>(n));
  for (std::int64_t gid = 0; gid < n; ++gid) {
    std::int64_t rem = gid;
    std::int64_t off = 0;
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      const auto sz = idx_sizes[static_cast<std::size_t>(d)];
      std::int64_t coord = (sz == 0) ? 0 : (rem % sz);
      rem = (sz == 0) ? 0 : (rem / sz);
      if (d == dim) {
        std::int64_t pick = ip[gid];
        const auto dim_sz = out_sizes[static_cast<std::size_t>(d)];
        if (pick < 0) pick += dim_sz;
        if (pick < 0 || pick >= dim_sz) {
          // Defer exact error behavior to canonical CPU scatter implementation.
          return true;
        }
        coord = pick;
      }
      off += coord * out_strides[static_cast<std::size_t>(d)];
    }
    offsets.push_back(off);
  }

  std::sort(offsets.begin(), offsets.end());
  for (std::size_t i = 1; i < offsets.size(); ++i) {
    if (offsets[i] == offsets[i - 1]) {
      return true;
    }
  }
  return false;
}

Series scatter(const Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter");
  require_same_platform(a, src, "scatter");
  Series out;
  Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  if (a.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const ScalarType out_dt = a.scalar_type();
    const bool supports_direct =
        native::mps::supports_scatter(out_dt, idx.scalar_type(), out_dt) &&
        (src.scalar_type() == out_dt || native::mps::supports_cast(src.scalar_type(), out_dt));
    const bool supports_f32 =
        native::mps::supports_scatter(ScalarType::Float32, idx.scalar_type(), ScalarType::Float32);
    const bool supports_f32_lane =
        supports_f32 &&
        mps_exact_f32_roundtrip_lane_dtype_(out_dt) &&
        (src.scalar_type() == ScalarType::Float32 ||
         mps_exact_f32_roundtrip_lane_dtype_(src.scalar_type()) ||
         native::mps::supports_cast(src.scalar_type(), ScalarType::Float32));
    if (strict_native && (idx.scalar_type() != ScalarType::Int64 || (!supports_direct && !supports_f32_lane))) {
      require_mps_native_dispatch_supported_("scatter");
    }
    if (supports_direct) {
      out = a.contiguous().clone();
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      if (mps_scatter_has_duplicate_targets_(out, dim, idx)) {
        const Series cpu_out =
            native::cpu::scatter(out.to(Platform::CPU), dim, idx.to(Platform::CPU), ss.to(Platform::CPU));
        out = cpu_out.to(Platform::MPS);
      } else {
        native::mps::scatter_set_(out, dim, idx, ss);
      }
    } else {
      require_mps_support_or_policy_(supports_f32_lane, "scatter");
      if (supports_f32_lane) {
        Series out_f = a.to(ScalarType::Float32).contiguous().clone();
        Series src_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
        if (mps_scatter_has_duplicate_targets_(out_f, dim, idx)) {
          const Series cpu_out =
              native::cpu::scatter(out_f.to(Platform::CPU), dim, idx.to(Platform::CPU), src_f.to(Platform::CPU));
          out = cpu_out.to(Platform::MPS).to(a.scalar_type());
        } else {
          native::mps::scatter_set_(out_f, dim, idx, src_f);
          out = (a.scalar_type() == ScalarType::Float32) ? out_f : out_f.to(a.scalar_type());
        }
      }
    }
  } else if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter(a, dim, index, src);
    out = cuda_scatter_gather_route_or_bridge_(
        "scatter",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::scatter(a, dim, index, src); });
  } else {
    require_backend_dispatch_supported_(a.platform(), "scatter");
    out = native::cpu::scatter(a, dim, index, src);
  }

  if (series_tape::recording_enabled()) {
    const bool ta = series_tape::tracks_slope(a);
    const bool ts = series_tape::tracks_slope(src);
    if (ta || ts) {
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "scatter";
      link->parents = {a, src};
      link->propagate = [a, src, idx, dim, ta, ts](const Series& out_sens) {
        if (ta) {
          Series zeros = Series::zeros(idx.sizes(), out_sens.scalar_type()).to(out_sens.platform());
          Series ga = scatter(out_sens, dim, idx, zeros);
          if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
          if (ga.platform() != a.platform()) ga = ga.to(a.platform());
          series_tape::accumulate_sensitivity(a, ga);
        }
        if (ts) {
          Series gs = gather(out_sens, dim, idx);
          gs = reduce_broadcasted_sensitivity_to_like_(gs, src, "scatter");
          if (gs.scalar_type() != src.scalar_type()) gs = gs.to(src.scalar_type());
          if (gs.platform() != src.platform()) gs = gs.to(src.platform());
          series_tape::accumulate_sensitivity(src, gs);
        }
      };
      series_tape::bind_source_link(out, std::move(link));
    }
  }
  return out;
}
Series& scatter_(Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_");
  require_same_platform(a, src, "scatter_");
  if (a.platform() == Platform::MPS) {
    a = scatter(a, dim, index, src);
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter(a, dim, index, src);
    a = cuda_scatter_gather_route_or_bridge_(
        "scatter_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::scatter(a, dim, index, src); });
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_");
  native::cpu::scatter_(a, dim, index, src);
  return a;
}

static native::mps::ScatterReduceOp mps_scatter_reduce_op_(const std::string& reduce) {
  if (reduce == "sum") return native::mps::ScatterReduceOp::Sum;
  if (reduce == "amax") return native::mps::ScatterReduceOp::AMax;
  if (reduce == "amin") return native::mps::ScatterReduceOp::AMin;
  PLUSS_FAIL("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
}

static double scatter_reduce_identity_(const std::string& reduce) {
  if (reduce == "sum") return 0.0;
  if (reduce == "mean") return 0.0;
  if (reduce == "amax") return -std::numeric_limits<double>::infinity();
  if (reduce == "amin") return +std::numeric_limits<double>::infinity();
  PLUSS_FAIL("scatter_reduce: unsupported reduce (expected sum|mean|amax|amin)");
}

static double scatter_reduce_identity_for_dtype_(const std::string& reduce, ScalarType dt) {
  if (reduce == "sum" || reduce == "mean") {
    return 0.0;
  }
  if (dt == ScalarType::Int32) {
    if (reduce == "amax") return static_cast<double>(std::numeric_limits<std::int32_t>::lowest());
    if (reduce == "amin") return static_cast<double>(std::numeric_limits<std::int32_t>::max());
  }
  if (dt == ScalarType::Int64) {
    if (reduce == "amax") return static_cast<double>(std::numeric_limits<std::int64_t>::lowest());
    if (reduce == "amin") return static_cast<double>(std::numeric_limits<std::int64_t>::max());
  }
  return scatter_reduce_identity_(reduce);
}

static void maybe_link_scatter_reduce_(
    const Series& out,
    const Series& a,
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce,
    bool include_self) {
  if (!series_tape::recording_enabled()) return;
  const bool ta = series_tape::tracks_slope(a);
  const bool ts = series_tape::tracks_slope(src);
  if (!ta && !ts) return;

  // Keep parity fail-loud until remaining scatter_reduce backward rules are implemented.
  if (reduce != "sum" && reduce != "mean" && reduce != "amax" && reduce != "amin") {
    PLUSS_FAIL(
        std::string("scatter_reduce: autodiff currently supports reduce in {'sum','mean','amax','amin'} only; got reduce='") +
        reduce + "'");
  }

  const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = std::string("scatter_reduce(") + reduce + ")";
  link->parents = {a, src};
  if (reduce == "sum") {
    link->propagate = [a, src, idx, dim, include_self, ta, ts](const Series& out_sens) {
      if (ta) {
        Series ga = include_self
            ? out_sens
            : scatter(out_sens, dim, idx, Series::zeros(idx.sizes(), out_sens.scalar_type()).to(out_sens.platform()));
        if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
        if (ga.platform() != a.platform()) ga = ga.to(a.platform());
        series_tape::accumulate_sensitivity(a, ga);
      }
      if (ts) {
        Series gs = gather(out_sens, dim, idx);
        gs = reduce_broadcasted_sensitivity_to_like_(gs, src, "scatter_reduce");
        if (gs.scalar_type() != src.scalar_type()) gs = gs.to(src.scalar_type());
        if (gs.platform() != src.platform()) gs = gs.to(src.platform());
        series_tape::accumulate_sensitivity(src, gs);
      }
    };
  } else if (reduce == "mean") {
    link->propagate = [a, src, idx, dim, include_self, ta, ts](const Series& out_sens) {
      Series n = include_self
          ? Series::ones(a.sizes(), out_sens.scalar_type()).to(out_sens.platform())
          : Series::zeros(a.sizes(), out_sens.scalar_type()).to(out_sens.platform());
      const Series ones_src = Series::ones(idx.sizes(), out_sens.scalar_type()).to(out_sens.platform());
      n = scatter_add(n, dim, idx, ones_src);
      const Series safe_n = n.clamp_min(1.0);
      const Series grad_distributed = out_sens.div(safe_n);

      if (ta) {
        Series ga = include_self
            ? grad_distributed
            : scatter(grad_distributed, dim, idx, Series::zeros(idx.sizes(), grad_distributed.scalar_type()).to(grad_distributed.platform()));
        if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
        if (ga.platform() != a.platform()) ga = ga.to(a.platform());
        series_tape::accumulate_sensitivity(a, ga);
      }
      if (ts) {
        Series gs = gather(grad_distributed, dim, idx);
        gs = reduce_broadcasted_sensitivity_to_like_(gs, src, "scatter_reduce");
        if (gs.scalar_type() != src.scalar_type()) gs = gs.to(src.scalar_type());
        if (gs.platform() != src.platform()) gs = gs.to(src.platform());
        series_tape::accumulate_sensitivity(src, gs);
      }
    };
  } else {
    link->propagate = [a, out, src, idx, dim, include_self, ta, ts](const Series& out_sens) {
      const Series n_mask =
          (a.eq(out).to(out_sens.scalar_type()))
              .scatter_add(dim, idx, src.expand(idx.sizes(), true).eq(out.gather(dim, idx)).to(out_sens.scalar_type()));
      const Series safe_n = n_mask.clamp_min(1.0);
      const Series grad_distributed = out_sens.div(safe_n);

      if (ta) {
        Series ga = a.eq(out).to(out_sens.scalar_type()).mul(grad_distributed);
        if (!include_self) {
          ga = scatter(ga, dim, idx, Series::zeros(idx.sizes(), ga.scalar_type()).to(ga.platform()));
        }
        if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
        if (ga.platform() != a.platform()) ga = ga.to(a.platform());
        series_tape::accumulate_sensitivity(a, ga);
      }
      if (ts) {
        const Series value = out.gather(dim, idx);
        const Series src_expanded = src.expand(idx.sizes(), true);
        Series gs_expanded =
            src_expanded.eq(value).to(out_sens.scalar_type()).mul(grad_distributed.gather(dim, idx));
        Series gs = reduce_broadcasted_sensitivity_to_like_(gs_expanded, src, "scatter_reduce");
        if (gs.scalar_type() != src.scalar_type()) gs = gs.to(src.scalar_type());
        if (gs.platform() != src.platform()) gs = gs.to(src.platform());
        series_tape::accumulate_sensitivity(src, gs);
      }
    };
  }
  series_tape::bind_source_link(out, std::move(link));
}

Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_add");
  require_same_platform(a, src, "scatter_add");
  const auto out_dt = promote_types(a.scalar_type(), src.scalar_type());
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    const bool supports_direct = native::mps::supports_scatter_reduce(
        out_dt, idx.scalar_type(), out_dt);
    const bool supports_f32 = native::mps::supports_scatter_reduce(
        ScalarType::Float32, idx.scalar_type(), ScalarType::Float32);
    const bool supports_f32_lane =
        supports_f32 &&
        mps_exact_f32_roundtrip_lane_dtype_(out_dt);
    if (strict_native && (idx.scalar_type() != ScalarType::Int64 || (!supports_direct && !supports_f32_lane))) {
      require_mps_native_dispatch_supported_("scatter_add");
    }
    require_mps_support_or_policy_(supports_direct || supports_f32_lane, "scatter_add");
    if (supports_direct) {
      Series out_native = a.to(out_dt).contiguous().clone();
      const Series ss_native = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out_native, dim, idx, ss_native, native::mps::ScatterReduceOp::Sum);
      out = out_native;
    } else if (supports_f32_lane) {
      Series out_f = a.to(ScalarType::Float32).contiguous().clone();
      const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
      native::mps::scatter_reduce_(out_f, dim, idx, ss_f, native::mps::ScatterReduceOp::Sum);
      out = (out_dt == ScalarType::Float32) ? out_f : out_f.to(out_dt);
    } else {
      Series out_cpu = a.to(out_dt).contiguous().to(Platform::CPU);
      const Series ss_cpu = src.expand(idx.sizes(), true).to(out_dt).contiguous().to(Platform::CPU);
      out_cpu = native::cpu::scatter_add(out_cpu, dim, idx.to(Platform::CPU), ss_cpu);
      out = out_cpu.to(Platform::MPS);
    }
  } else if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_add(a, dim, index, src);
    out = cuda_scatter_gather_route_or_bridge_(
        "scatter_add",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::scatter_add(a, dim, index, src); });
  } else {
    require_backend_dispatch_supported_(a.platform(), "scatter_add");
    out = native::cpu::scatter_add(a.clone(), dim, index, src);
  }

  if (series_tape::recording_enabled()) {
    const bool ta = series_tape::tracks_slope(a);
    const bool ts = series_tape::tracks_slope(src);
    if (ta || ts) {
      auto idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "scatter_add";
      link->parents = {a, src};
      link->propagate = [a, src, idx, dim](const Series& out_sens) {
        if (series_tape::tracks_slope(a)) {
          Series ga = out_sens;
          if (ga.scalar_type() != a.scalar_type()) ga = ga.to(a.scalar_type());
          if (ga.platform() != a.platform()) ga = ga.to(a.platform());
          series_tape::accumulate_sensitivity(a, ga);
        }
        if (series_tape::tracks_slope(src)) {
          Series gs = gather(out_sens, dim, idx);
          gs = reduce_broadcasted_sensitivity_to_like_(gs, src, "scatter_add");
          if (gs.scalar_type() != src.scalar_type()) gs = gs.to(src.scalar_type());
          if (gs.platform() != src.platform()) gs = gs.to(src.platform());
          series_tape::accumulate_sensitivity(src, gs);
        }
      };
      series_tape::bind_source_link(out, std::move(link));
    }
  }
  return out;
}
Series& scatter_add_(Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_add_");
  require_same_platform(a, src, "scatter_add_");
  if (a.platform() == Platform::MPS) {
    a = scatter_add(a, dim, index, src).to(a.scalar_type());
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_add(a, dim, index, src);
    a = cuda_scatter_gather_route_or_bridge_(
            "scatter_add_",
            native_enabled,
            native_strict,
            route_supported,
            [&]() { return native::cpu::scatter_add(a, dim, index, src); })
            .to(a.scalar_type());
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_add_");
  native::cpu::scatter_add_inplace(a, dim, index, src);
  return a;
}
Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self) {
  require_same_platform(a, index, "scatter_reduce");
  require_same_platform(a, src, "scatter_reduce");
  const auto out_dt = promote_types(a.scalar_type(), src.scalar_type());
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    if (reduce == "mean") {
      if (strict_native) {
        require_mps_native_dispatch_supported_("scatter_reduce");
      }
      const Series idx_cpu =
          (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous().to(Platform::CPU);
      const Series src_cpu = src.expand(index.sizes(), true).to(out_dt).contiguous().to(Platform::CPU);
      const Series a_cpu = a.to(out_dt).contiguous().to(Platform::CPU);
      out = native::cpu::scatter_reduce(a_cpu, dim, idx_cpu, src_cpu, reduce, include_self).to(Platform::MPS);
      maybe_link_scatter_reduce_(out, a, dim, index, src, reduce, include_self);
      return out;
    }
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    const auto rop = mps_scatter_reduce_op_(reduce);
    const bool supports_direct = native::mps::supports_scatter_reduce(out_dt, idx.scalar_type(), out_dt);
    const bool supports_f32 = native::mps::supports_scatter_reduce(
        ScalarType::Float32, idx.scalar_type(), ScalarType::Float32);
    const bool supports_f32_lane =
        supports_f32 &&
        mps_exact_f32_roundtrip_lane_dtype_(out_dt);
    if (strict_native && (idx.scalar_type() != ScalarType::Int64 || (!supports_direct && !supports_f32_lane))) {
      require_mps_native_dispatch_supported_("scatter_reduce");
    }
    if (supports_direct) {
      out = include_self
                     ? a.to(out_dt).clone().contiguous()
                     : Series::full(a.sizes(), scatter_reduce_identity_for_dtype_(reduce, out_dt), out_dt)
                           .to(a.platform())
                           .contiguous();
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out, dim, idx, ss, rop);
    } else {
      require_mps_support_or_policy_(supports_f32_lane, "scatter_reduce");
      if (supports_f32_lane) {
        Series out_f = include_self
                         ? a.to(ScalarType::Float32).clone().contiguous()
                         : Series::full(a.sizes(), scatter_reduce_identity_(reduce), ScalarType::Float32)
                               .to(a.platform())
                               .contiguous();
        const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
        native::mps::scatter_reduce_(out_f, dim, idx, ss_f, rop);
        out = out_f.to(out_dt);
      }
    }
  } else if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_reduce(a, dim, index, src, reduce);
    out = cuda_scatter_gather_route_or_bridge_(
        "scatter_reduce",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self); });
  } else {
    require_backend_dispatch_supported_(a.platform(), "scatter_reduce");
    out = native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self);
  }
  maybe_link_scatter_reduce_(out, a, dim, index, src, reduce, include_self);
  return out;
}
Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial) {
  require_same_platform(a, index, "scatter_reduce");
  require_same_platform(a, src, "scatter_reduce");
  const auto out_dt = promote_types(a.scalar_type(), src.scalar_type());
  Series out;
  if (a.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    if (reduce == "mean") {
      if (strict_native) {
        require_mps_native_dispatch_supported_("scatter_reduce");
      }
      const Series idx_cpu =
          (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous().to(Platform::CPU);
      const Series src_cpu = src.expand(index.sizes(), true).to(out_dt).contiguous().to(Platform::CPU);
      const Series a_cpu = a.to(out_dt).contiguous().to(Platform::CPU);
      out = native::cpu::scatter_reduce(a_cpu, dim, idx_cpu, src_cpu, reduce, include_self, has_initial, initial).to(Platform::MPS);
      maybe_link_scatter_reduce_(out, a, dim, index, src, reduce, include_self);
      return out;
    }
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    const auto rop = mps_scatter_reduce_op_(reduce);
    const bool supports_direct = native::mps::supports_scatter_reduce(out_dt, idx.scalar_type(), out_dt);
    const bool supports_f32 = native::mps::supports_scatter_reduce(
        ScalarType::Float32, idx.scalar_type(), ScalarType::Float32);
    const bool supports_f32_lane =
        supports_f32 &&
        mps_exact_f32_roundtrip_lane_dtype_(out_dt);
    if (strict_native && (idx.scalar_type() != ScalarType::Int64 || (!supports_direct && !supports_f32_lane))) {
      require_mps_native_dispatch_supported_("scatter_reduce");
    }
    if (supports_direct) {
      out = has_initial
                     ? Series::full(a.sizes(), initial, out_dt).to(a.platform()).contiguous()
                     : (include_self
                          ? a.to(out_dt).clone().contiguous()
                          : Series::full(a.sizes(), scatter_reduce_identity_for_dtype_(reduce, out_dt), out_dt)
                                .to(a.platform())
                                .contiguous());
      if (has_initial && include_self) {
        const Series aa = a.to(out_dt);
        if (reduce == "sum") out = add(out, aa);
        else if (reduce == "amax") out = maximum(out, aa);
        else if (reduce == "amin") out = minimum(out, aa);
        else PLUSS_FAIL("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
      }
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out, dim, idx, ss, rop);
    } else {
      require_mps_support_or_policy_(supports_f32_lane, "scatter_reduce");
      if (supports_f32_lane) {
        Series out_f = has_initial
                         ? Series::full(a.sizes(), initial, ScalarType::Float32).to(a.platform()).contiguous()
                         : (include_self
                              ? a.to(ScalarType::Float32).clone().contiguous()
                              : Series::full(a.sizes(), scatter_reduce_identity_(reduce), ScalarType::Float32)
                                    .to(a.platform())
                                    .contiguous());
        if (has_initial && include_self) {
          const Series aa_f = a.to(ScalarType::Float32);
          if (reduce == "sum") out_f = add(out_f, aa_f);
          else if (reduce == "amax") out_f = maximum(out_f, aa_f);
          else if (reduce == "amin") out_f = minimum(out_f, aa_f);
          else PLUSS_FAIL("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
        }
        const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
        native::mps::scatter_reduce_(out_f, dim, idx, ss_f, rop);
        out = out_f.to(out_dt);
      }
    }
  } else if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_reduce(a, dim, index, src, reduce);
    out = cuda_scatter_gather_route_or_bridge_(
        "scatter_reduce",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self, has_initial, initial); });
  } else {
    require_backend_dispatch_supported_(a.platform(), "scatter_reduce");
    out = native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self, has_initial, initial);
  }
  maybe_link_scatter_reduce_(out, a, dim, index, src, reduce, include_self);
  return out;
}
Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self) {
  require_same_platform(a, index, "scatter_reduce_");
  require_same_platform(a, src, "scatter_reduce_");
  if (a.platform() == Platform::MPS) {
    a = scatter_reduce(a, dim, index, src, reduce, include_self).to(a.scalar_type());
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_reduce(a, dim, index, src, reduce);
    a = cuda_scatter_gather_route_or_bridge_(
            "scatter_reduce_",
            native_enabled,
            native_strict,
            route_supported,
            [&]() { return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self); })
            .to(a.scalar_type());
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_reduce_");
  native::cpu::scatter_reduce_inplace(a, dim, index, src, reduce, include_self);
  return a;
}
Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial) {
  require_same_platform(a, index, "scatter_reduce_");
  require_same_platform(a, src, "scatter_reduce_");
  if (a.platform() == Platform::MPS) {
    a = scatter_reduce(a, dim, index, src, reduce, include_self, has_initial, initial).to(a.scalar_type());
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_scatter_reduce(a, dim, index, src, reduce);
    a = cuda_scatter_gather_route_or_bridge_(
            "scatter_reduce_",
            native_enabled,
            native_strict,
            route_supported,
            [&]() { return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self, has_initial, initial); })
            .to(a.scalar_type());
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_reduce_");
  native::cpu::scatter_reduce_inplace(a, dim, index, src, reduce, include_self, has_initial, initial);
  return a;
}

Series gather(const Series& input, std::int64_t dim, const Series& index) {
  require_same_platform(input, index, "gather");
  const auto nd = input.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "gather: dim out of range");
  if (input.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const bool index_native_lane =
        index.scalar_type() == ScalarType::Int64 ||
        native::mps::supports_cast(index.scalar_type(), ScalarType::Int64);
    const bool input_native_lane =
        native::mps::supports_gather(input.scalar_type(), ScalarType::Int64) ||
        mps_exact_f32_roundtrip_lane_dtype_(input.scalar_type());
    if (strict_native && (!index_native_lane || !input_native_lane)) {
      require_mps_native_dispatch_supported_("gather");
    }
  }
  auto in = input.contiguous();
  if (input.platform() == Platform::MPS && index.scalar_type() != ScalarType::Int64) {
    require_mps_support_or_policy_(
        native::mps::supports_cast(index.scalar_type(), ScalarType::Int64),
        "gather");
  }
  auto idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  Series out;
  if (input.platform() == Platform::MPS) {
    if (native::mps::supports_gather(in.scalar_type(), idx.scalar_type())) {
      out = Series::empty(std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end()),
                          in.scalar_type(),
                          in.platform());
      native::mps::gather(in, dim, idx, out);
    } else {
      const bool supports_f32 = native::mps::supports_gather(ScalarType::Float32, idx.scalar_type());
      require_mps_support_or_policy_(supports_f32, "gather");
      if (supports_f32) {
        if (in.scalar_type() != ScalarType::Float32) {
          const bool cast_supported =
              mps_exact_f32_roundtrip_lane_dtype_(in.scalar_type()) ||
              (native::mps::supports_cast(in.scalar_type(), ScalarType::Float32) &&
               native::mps::supports_cast(ScalarType::Float32, input.scalar_type()));
          require_mps_support_or_policy_(cast_supported, "gather");
        }
        Series in_f = in.to(ScalarType::Float32);
        Series out_f = Series::empty(std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end()),
                                     ScalarType::Float32,
                                     in.platform());
        native::mps::gather(in_f, dim, idx, out_f);
        out = out_f.to(input.scalar_type());
      }
    }
  } else if (input.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scatter_gather_enabled();
    const bool native_strict = native::cuda::native_scatter_gather_strict();
    const bool route_supported = native::cuda::supports_gather(input, dim, index);
    out = cuda_scatter_gather_route_or_bridge_(
        "gather",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return native::cpu::gather(in, dim, idx); });
  } else {
    require_backend_dispatch_supported_(input.platform(), "gather");
    out = native::cpu::gather(in, dim, idx);
  }

  maybe_link_unary_op_(
      out,
      input,
      "gather",
      [input, idx, dim](const Series& out_sens) {
        Series sens = out_sens;
        if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
        if (sens.platform() != input.platform()) sens = sens.to(input.platform());
        Series base = Series::zeros(input.sizes(), input.scalar_type()).to(input.platform());
        return scatter_add(base, dim, idx, sens);
      });
  return out;
}


Series maximum(const Series& a, const Series& b) {
  require_same_platform(a, b, "maximum");
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  if (a.platform() == Platform::MPS && out.scalar_type() == ScalarType::Float32) {
    native::mps::maximum(aa, bb, out);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_binary_elementwise(aa, bb);
    return cuda_elementwise_route_or_bridge_(
        "maximum",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          native::cpu::maximum(aa, bb, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "maximum");
  native::cpu::maximum(aa, bb, out);
  return out;
}

Series minimum(const Series& a, const Series& b) {
  require_same_platform(a, b, "minimum");
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  if (a.platform() == Platform::MPS && out.scalar_type() == ScalarType::Float32) {
    native::mps::minimum(aa, bb, out);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_binary_elementwise(aa, bb);
    return cuda_elementwise_route_or_bridge_(
        "minimum",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          native::cpu::minimum(aa, bb, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "minimum");
  native::cpu::minimum(aa, bb, out);
  return out;
}

Series atan2(const Series& a, const Series& b) {
  require_same_platform(a, b, "atan2");
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    auto bb = b.contiguous().to(ScalarType::Float32);
    const Series zero = scalar_like_(aa, 0.0);
    const Series pi = scalar_like_(aa, 3.14159265358979323846);
    const Series half_pi = scalar_like_(aa, 1.57079632679489661923);
    const Series neg_one = scalar_like_(aa, -1.0);

    Series out = atan(div(aa, bb));
    out = where(logical_and(lt(bb, zero), ge(aa, zero)), add(out, pi), out);
    out = where(logical_and(lt(bb, zero), lt(aa, zero)), sub(out, pi), out);
    out = where(logical_and(eq(bb, zero), gt(aa, zero)), half_pi, out);
    out = where(logical_and(eq(bb, zero), lt(aa, zero)), mul(half_pi, neg_one), out);
    out = where(logical_and(eq(bb, zero), eq(aa, zero)), zero, out);
    return out;
  }
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const Series aa = a.contiguous().to(out.scalar_type());
    const Series bb = b.contiguous().to(out.scalar_type());
    const bool route_supported = native::cuda::supports_binary_elementwise(aa, bb);
    return cuda_elementwise_route_or_bridge_(
        "atan2",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          native::cpu::atan2(aa, bb, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "atan2");
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  native::cpu::atan2(aa, bb, out);
  return out;
}

Series fmod(const Series& a, const Series& b) {
  require_same_platform(a, b, "fmod");
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    auto bb = b.contiguous().to(ScalarType::Float32);
    return sub(aa, mul(trunc(div(aa, bb)), bb));
  }
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const Series aa = a.contiguous().to(out.scalar_type());
    const Series bb = b.contiguous().to(out.scalar_type());
    const bool route_supported = native::cuda::supports_binary_elementwise(aa, bb);
    return cuda_elementwise_route_or_bridge_(
        "fmod",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          native::cpu::fmod(aa, bb, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "fmod");
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  native::cpu::fmod(aa, bb, out);
  return out;
}

Series remainder(const Series& a, const Series& b) {
  require_same_platform(a, b, "remainder");
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    auto bb = b.contiguous().to(ScalarType::Float32);
    return sub(aa, mul(floor(div(aa, bb)), bb));
  }
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const Series aa = a.contiguous().to(out.scalar_type());
    const Series bb = b.contiguous().to(out.scalar_type());
    const bool route_supported = native::cuda::supports_binary_elementwise(aa, bb);
    return cuda_elementwise_route_or_bridge_(
        "remainder",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          native::cpu::remainder(aa, bb, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "remainder");
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  native::cpu::remainder(aa, bb, out);
  return out;
}

Series rsqrt(const Series& a) {
  Series out = (a.platform() == Platform::MPS)
                   ? reciprocal(sqrt(a.contiguous().to(ScalarType::Float32)))
                   : Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  if (a.platform() != Platform::MPS) {
    if (a.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_elementwise_enabled();
      const bool native_strict = native::cuda::native_elementwise_strict();
      const bool route_supported = native::cuda::supports_unary_elementwise(a);
      out = cuda_elementwise_route_or_bridge_(
          "rsqrt",
          native_enabled,
          native_strict,
          route_supported,
          [&]() {
            const Series aa = a.contiguous().to(out.scalar_type());
            native::cpu::rsqrt(aa, out);
            return out;
          });
    } else {
      require_backend_dispatch_supported_(a.platform(), "rsqrt");
      auto aa = a.contiguous().to(out.scalar_type());
      native::cpu::rsqrt(aa, out);
    }
  }
  maybe_link_unary_op_(out, a, [a](const Series& g) {
    const Series y = rsqrt(a);
    return g.mul(-0.5).mul(mul(mul(y, y), y));
  });
  return out;
}
Series log2(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series ln2 = scalar_like_(aa, 0.69314718055994530942);
    return div(log(aa), ln2);
  }
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "log2",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series aa = a.contiguous().to(out.scalar_type());
          native::cpu::log2(aa, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "log2");
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::log2(aa, out);
  return out;
}
Series log10(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series ln10 = scalar_like_(aa, 2.30258509299404568402);
    return div(log(aa), ln10);
  }
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "log10",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series aa = a.contiguous().to(out.scalar_type());
          native::cpu::log10(aa, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "log10");
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::log10(aa, out);
  return out;
}
Series exp2(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series ln2 = scalar_like_(aa, 0.69314718055994530942);
    return exp(mul(aa, ln2));
  }
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "exp2",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series aa = a.contiguous().to(out.scalar_type());
          native::cpu::exp2(aa, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "exp2");
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::exp2(aa, out);
  return out;
}
Series cbrt(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series one_third = scalar_like_(aa, 0.33333333333333333333);
    return pow(aa, one_third);
  }
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "cbrt",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series aa = a.contiguous().to(out.scalar_type());
          native::cpu::cbrt(aa, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "cbrt");
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::cbrt(aa, out);
  return out;
}
Series square(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    auto aa = a.contiguous();
    out = mul(aa, aa);
  } else {
    if (a.platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_elementwise_enabled();
      const bool native_strict = native::cuda::native_elementwise_strict();
      const bool route_supported = native::cuda::supports_unary_elementwise(a);
      out = cuda_elementwise_route_or_bridge_(
          "square",
          native_enabled,
          native_strict,
          route_supported,
          [&]() {
            Series out_local = Series::empty(a.sizes(), a.scalar_type(), a.platform());
            auto aa = a.contiguous();
            native::cpu::square(aa, out_local);
            return out_local;
          });
    } else {
      require_backend_dispatch_supported_(a.platform(), "square");
      out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
      auto aa = a.contiguous();
      native::cpu::square(aa, out);
    }
  }
  if (series_tape::recording_enabled() &&
      series_tape::tracks_slope(a) &&
      a.platform() == Platform::MPS) {
    // Snapshot tracked MPS inputs used in backward formulas so gradient
    // replay does not depend on transient alias lifetimes.
    const Series a_snapshot = a.clone();
    maybe_link_unary_op_(out, a, [a_snapshot](const Series& g) {
      Series grad = g.mul(2.0).mul(a_snapshot);
      if (a_snapshot.platform() == Platform::MPS) {
        // Force materialization before parent accumulation to avoid transient
        // MPS alias/cache hazards across adjacent backward nodes.
        (void)static_cast<const Series&>(grad).raw_data();
      }
      return grad;
    });
  } else {
    maybe_link_unary_op_(out, a, [a](const Series& g) {
      return g.mul(2.0).mul(a);
    });
  }
  return out;
}

Series nan_to_num(const Series& a, double nan, double posinf, double neginf) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    auto aa = a.contiguous();
    const Series nan_s = scalar_like_(aa, nan);
    const Series posinf_s = scalar_like_(aa, posinf);
    const Series neginf_s = scalar_like_(aa, neginf);
    const Series zero = scalar_like_(aa, 0.0);

    Series out = where(isnan(aa), nan_s, aa);
    const Series inf_mask = isinf(aa);
    out = where(logical_and(inf_mask, gt(aa, zero)), posinf_s, out);
    out = where(logical_and(inf_mask, lt(aa, zero)), neginf_s, out);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "nan_to_num",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
          auto aa = a.contiguous();
          native::cpu::nan_to_num(aa, nan, posinf, neginf, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "nan_to_num");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::nan_to_num(aa, nan, posinf, neginf, out);
  return out;
}
Series& nan_to_num_(Series& a, double nan, double posinf, double neginf) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    a = nan_to_num(a, nan, posinf, neginf);
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    a = nan_to_num(a, nan, posinf, neginf);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "nan_to_num_");
  auto aa = a.contiguous();
  Series out = Series::empty(aa.sizes(), aa.scalar_type(), aa.platform());
  native::cpu::nan_to_num(aa, nan, posinf, neginf, out);
  aa.copy_(out);
  // copy back into original view if needed
  if (!a.is_contiguous()) a.copy_(aa);
  return a;
}

Series clamp_min(const Series& a, double lo) {
  if (a.platform() == Platform::MPS) {
    if (a.scalar_type() == ScalarType::Float32) {
      return maximum(a, scalar_like_(a, lo));
    }
    const bool supports_f32_lane =
        mps_exact_f32_roundtrip_lane_dtype_(a.scalar_type()) ||
        a.scalar_type() == ScalarType::Int32;
    require_mps_support_or_policy_(supports_f32_lane, "clamp_min");
    if (supports_f32_lane) {
      const Series aa_f = a.to(ScalarType::Float32).contiguous();
      return maximum(aa_f, scalar_like_(aa_f, lo)).to(a.scalar_type());
    }
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "clamp_min",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
          auto aa = a.contiguous();
          native::cpu::clamp_min(aa, lo, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_min");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::clamp_min(aa, lo, out);
  return out;
}
Series& clamp_min_(Series& a, double lo) {
  if (a.platform() == Platform::MPS) {
    a = clamp_min(a, lo);
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    a = clamp_min(a, lo);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_min_");
  auto aa = a.contiguous();
  Series out = Series::empty(aa.sizes(), aa.scalar_type(), aa.platform());
  native::cpu::clamp_min(aa, lo, out);
  aa.copy_(out);
  if (!a.is_contiguous()) a.copy_(aa);
  return a;
}

Series clamp_max(const Series& a, double hi) {
  if (a.platform() == Platform::MPS) {
    if (a.scalar_type() == ScalarType::Float32) {
      return minimum(a, scalar_like_(a, hi));
    }
    const bool supports_f32_lane =
        mps_exact_f32_roundtrip_lane_dtype_(a.scalar_type()) ||
        a.scalar_type() == ScalarType::Int32;
    require_mps_support_or_policy_(supports_f32_lane, "clamp_max");
    if (supports_f32_lane) {
      const Series aa_f = a.to(ScalarType::Float32).contiguous();
      return minimum(aa_f, scalar_like_(aa_f, hi)).to(a.scalar_type());
    }
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_elementwise_enabled();
    const bool native_strict = native::cuda::native_elementwise_strict();
    const bool route_supported = native::cuda::supports_unary_elementwise(a);
    return cuda_elementwise_route_or_bridge_(
        "clamp_max",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
          auto aa = a.contiguous();
          native::cpu::clamp_max(aa, hi, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_max");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::clamp_max(aa, hi, out);
  return out;
}
Series& clamp_max_(Series& a, double hi) {
  if (a.platform() == Platform::MPS) {
    a = clamp_max(a, hi);
    return a;
  }
  if (a.platform() == Platform::CUDA) {
    a = clamp_max(a, hi);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_max_");
  auto aa = a.contiguous();
  Series out = Series::empty(aa.sizes(), aa.scalar_type(), aa.platform());
  native::cpu::clamp_max(aa, hi, out);
  aa.copy_(out);
  if (!a.is_contiguous()) a.copy_(aa);
  return a;
}


Series masked_scatter(const Series& a, const Series& mask, const Series& src) {
  require_same_platform(a, mask, "masked_scatter");
  require_same_platform(a, src, "masked_scatter");
  Series out = a.clone();
  masked_scatter_(out, mask, src);
  return out;
}

Series& masked_scatter_(Series& a, const Series& mask, const Series& src) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "masked_scatter_: mask must be bool");
  require_same_platform(a, mask, "masked_scatter_");
  require_same_platform(a, src, "masked_scatter_");
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(a.is_contiguous(), "masked_scatter_: requires contiguous Series on MPS");
    auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
    PLUSS_CHECK(a.sizes().size() == shape.size(),
                "masked_scatter_: shape mismatch (require a already broadcasted)");
    PLUSS_CHECK(std::equal(a.sizes().begin(), a.sizes().end(), shape.begin()),
                "masked_scatter_: shape mismatch (require a already broadcasted)");
    const std::array<std::int64_t, 1> flat_shape{{a.numel()}};
    const Series mm_flat = mask.expand(shape, true).contiguous().reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    const Series nz2d = nonzero(mm_flat);
    const std::int64_t count = nz2d.numel();
    PLUSS_CHECK(src.numel() == count, "masked_scatter_: src numel must match number of True in mask");
    if (count == 0) {
      return a;
    }
    const Series idx = nz2d.to(Platform::CPU).squeeze(-1).contiguous().to(Platform::MPS);
    const std::array<std::int64_t, 1> src_shape{{count}};
    const Series ss = src.contiguous().to(a.scalar_type()).reshape(std::span<const std::int64_t>(src_shape.data(), src_shape.size()));
    Series a_flat = a.reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    a_flat = scatter(a_flat, 0, idx, ss);
    (void)static_cast<const Series&>(a_flat).raw_data();
    a = a_flat.reshape(shape);
    return a;
  }
  auto cpu_apply = [&](Series& target) -> Series& {
    auto shape = SeriesIterator::broadcast_shape(target.sizes(), mask.sizes());
    PLUSS_CHECK(std::equal(target.sizes().begin(), target.sizes().end(), shape.begin(), shape.end()),
                "masked_scatter_: shape mismatch (require a already broadcasted)");
    auto mm = mask.expand(shape, true).contiguous();
    const bool* mp = reinterpret_cast<const bool*>(mm.raw_data());
    const auto n = target.numel();
    std::int64_t count = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      if (mp[i]) ++count;
    }
    PLUSS_CHECK(src.numel() == count, "masked_scatter_: src numel must match number of True in mask");
    auto ss = src.contiguous().to(target.scalar_type());

    const auto item = target.dtype().itemsize;
    std::byte* ap = reinterpret_cast<std::byte*>(target.raw_mutable_data());
    const std::byte* sp = ss.raw_data();
    std::int64_t j = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      if (!mp[i]) continue;
      std::memcpy(ap + i * item, sp + j * item, (std::size_t)item);
      ++j;
    }
    return target;
  };

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_masking_enabled();
    const bool native_strict = native::cuda::native_masking_strict();
    const bool route_supported = native::cuda::supports_masked_scatter(a, mask, src);
    a = cuda_masking_route_or_bridge_(
        "masked_scatter_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = a.clone();
          cpu_apply(out);
          return out;
        });
    return a;
  }

  require_backend_dispatch_supported_(a.platform(), "masked_scatter_");
  cpu_apply(a);
  return a;
}

static std::int64_t read_i64_element(const Series& x, std::int64_t i) {
  switch (x.scalar_type()) {
    case ScalarType::Bool: return reinterpret_cast<const std::uint8_t*>(x.raw_data())[i] ? 1 : 0;
    case ScalarType::UInt8: return reinterpret_cast<const std::uint8_t*>(x.raw_data())[i];
    case ScalarType::UInt16: return reinterpret_cast<const std::uint16_t*>(x.raw_data())[i];
    case ScalarType::UInt32: return static_cast<std::int64_t>(reinterpret_cast<const std::uint32_t*>(x.raw_data())[i]);
    case ScalarType::UInt64: return static_cast<std::int64_t>(reinterpret_cast<const std::uint64_t*>(x.raw_data())[i]);
    case ScalarType::Int8: return reinterpret_cast<const std::int8_t*>(x.raw_data())[i];
    case ScalarType::Int16: return reinterpret_cast<const std::int16_t*>(x.raw_data())[i];
    case ScalarType::Int32: return reinterpret_cast<const std::int32_t*>(x.raw_data())[i];
    case ScalarType::Int64: return reinterpret_cast<const std::int64_t*>(x.raw_data())[i];
    default: PLUSS_FAIL("read_i64_element: expected integral/bool dtype");
  }
}

Series unique(const Series& a, bool sorted) {
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_counting_enabled();
    const bool native_strict = native::cuda::native_counting_strict();
    const bool route_supported = native::cuda::supports_unique_counting(a, sorted);
    return cuda_counting_route_or_bridge_(
        "unique",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series cpu = a.to(Platform::CPU);
          return unique(cpu, sorted).to(Platform::CUDA);
        });
  }

  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(sorted, "unique: MPS path currently requires sorted=true");
    auto x = (a.dim() == 1) ? a.contiguous() : a.reshape({a.numel()}).contiguous();
    PLUSS_CHECK(
        x.scalar_type() == ScalarType::Int64 || x.scalar_type() == ScalarType::Int32,
        "unique: MPS path currently supports int32/int64 tensors only");
    if (x.scalar_type() == ScalarType::Int32) {
      x = x.to(ScalarType::Int64);
    }
    auto counts = bincount(x, 0);
    if (counts.numel() == 0) {
      return Series::empty({0}, ScalarType::Int64, Platform::MPS);
    }
    auto present = counts.to(ScalarType::Float32).gt(0.0);
    auto nz = nonzero(present);
    if (nz.numel() == 0) {
      return Series::empty({0}, ScalarType::Int64, Platform::MPS);
    }
    return nz.to(Platform::CPU).squeeze(-1).to(ScalarType::Int64).to(Platform::MPS);
  }

  auto x = (a.dim() == 1) ? a.contiguous() : a.reshape({a.numel()}).contiguous();
  // For float NaN: treat NaNs as equal for uniqueness, similar to Torch's default behavior in CPU path.
  // We'll normalize by mapping NaNs to a single sentinel during comparison but keep original values.
  if (x.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(x.raw_data());
    std::vector<float> v(p, p + x.numel());
    std::vector<float> out_vec;
    if (sorted) {
      std::sort(v.begin(), v.end(), [](float u,float w){
        const bool un = std::isnan(u), wn = std::isnan(w);
        if (un && wn) return false;
        if (un) return false;
        if (wn) return true;
        return u < w;
      });
      v.erase(std::unique(v.begin(), v.end(), [](float u,float w){
        const bool un = std::isnan(u), wn = std::isnan(w);
        if (un && wn) return true;
        return u == w;
      }), v.end());
      out_vec = std::move(v);
    } else {
      out_vec.reserve(v.size());
      std::unordered_set<std::uint32_t> seen;
      seen.reserve(v.size() * 2 + 1);
      for (float val : v) {
        std::uint32_t bits = 0;
        if (std::isnan(val)) {
          bits = 0x7fc00000U;
        } else if (val == 0.0f) {
          bits = 0U;
        } else {
          std::memcpy(&bits, &val, sizeof(bits));
        }
        if (seen.insert(bits).second) {
          out_vec.push_back(val);
        }
      }
    }
    const std::array<std::int64_t, 1> os{{(std::int64_t)out_vec.size()}};
    Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Float32, x.platform());
    std::memcpy(out.raw_mutable_data(), out_vec.data(), out_vec.size()*sizeof(float));
    return out;
  }
  if (x.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(x.raw_data());
    std::vector<double> v(p, p + x.numel());
    std::vector<double> out_vec;
    if (sorted) {
      std::sort(v.begin(), v.end(), [](double u,double w){
        const bool un = std::isnan(u), wn = std::isnan(w);
        if (un && wn) return false;
        if (un) return false;
        if (wn) return true;
        return u < w;
      });
      v.erase(std::unique(v.begin(), v.end(), [](double u,double w){
        const bool un = std::isnan(u), wn = std::isnan(w);
        if (un && wn) return true;
        return u == w;
      }), v.end());
      out_vec = std::move(v);
    } else {
      out_vec.reserve(v.size());
      std::unordered_set<std::uint64_t> seen;
      seen.reserve(v.size() * 2 + 1);
      for (double val : v) {
        std::uint64_t bits = 0;
        if (std::isnan(val)) {
          bits = 0x7ff8000000000000ULL;
        } else if (val == 0.0) {
          bits = 0ULL;
        } else {
          std::memcpy(&bits, &val, sizeof(bits));
        }
        if (seen.insert(bits).second) {
          out_vec.push_back(val);
        }
      }
    }
    const std::array<std::int64_t, 1> os{{(std::int64_t)out_vec.size()}};
    Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Float64, x.platform());
    std::memcpy(out.raw_mutable_data(), out_vec.data(), out_vec.size()*sizeof(double));
    return out;
  }
  // Integer/bool path
  auto st = x.scalar_type();
  PLUSS_CHECK(dtype_of(st).is_integral() || st == ScalarType::Bool, "unique: dtype not supported");
  std::vector<std::int64_t> vv;
  vv.reserve((std::size_t)x.numel());
  for (std::int64_t i=0;i<x.numel();++i) vv.push_back(read_i64_element(x, i));
  std::vector<std::int64_t> out_v;
  if (sorted) {
    std::sort(vv.begin(), vv.end());
    vv.erase(std::unique(vv.begin(), vv.end()), vv.end());
    out_v = std::move(vv);
  } else {
    out_v.reserve(vv.size());
    std::unordered_set<std::int64_t> seen;
    seen.reserve(vv.size() * 2 + 1);
    for (std::int64_t val : vv) {
      if (seen.insert(val).second) {
        out_v.push_back(val);
      }
    }
  }
  const std::array<std::int64_t, 1> os{{(std::int64_t)out_v.size()}};
  Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Int64, x.platform());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  for (std::size_t i=0;i<out_v.size();++i) op[i]=out_v[i];
  return out;
}

namespace {

inline std::uint64_t unique_mix64_(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

inline std::uint64_t canonical_f32_bits_(float v) {
  if (std::isnan(v)) return 0x7fc00000ULL;
  std::uint32_t bits = 0;
  std::memcpy(&bits, &v, sizeof(bits));
  return static_cast<std::uint64_t>(bits);
}

inline std::uint64_t canonical_f64_bits_(double v) {
  if (std::isnan(v)) return 0x7ff8000000000000ULL;
  std::uint64_t bits = 0;
  std::memcpy(&bits, &v, sizeof(bits));
  return bits;
}

struct UniqueDimResult_ {
  Series values{};
  Series inverse{};
  Series counts{};
};

inline std::uint64_t hash_row_bytes_fast_(const std::uint8_t* p, std::size_t n) {
  std::uint64_t h = 0xcbf29ce484222325ULL;
  std::size_t k = 0;
  for (; k + sizeof(std::uint64_t) <= n; k += sizeof(std::uint64_t)) {
    std::uint64_t word = 0;
    std::memcpy(&word, p + k, sizeof(word));
    h ^= unique_mix64_(word);
    h *= 0x100000001b3ULL;
  }
  for (; k < n; ++k) {
    h ^= static_cast<std::uint64_t>(p[k]);
    h *= 0x100000001b3ULL;
  }
  return h;
}

UniqueDimResult_ unique_dim_impl_(const Series& a,
                                  std::int64_t dim,
                                  bool sorted,
                                  bool need_inverse,
                                  bool need_counts) {
  if (a.platform() == Platform::CUDA) {
    UniqueDimResult_ cpu = unique_dim_impl_(a.to(Platform::CPU), dim, sorted, need_inverse, need_counts);
    UniqueDimResult_ out{};
    out.values = cpu.values.to(Platform::CUDA);
    if (need_inverse) out.inverse = cpu.inverse.to(Platform::CUDA);
    if (need_counts) out.counts = cpu.counts.to(Platform::CUDA);
    return out;
  }

  if (a.platform() == Platform::MPS) {
    UniqueDimResult_ cpu = unique_dim_impl_(a.to(Platform::CPU), dim, sorted, need_inverse, need_counts);
    UniqueDimResult_ out{};
    out.values = cpu.values.to(Platform::MPS);
    if (need_inverse) out.inverse = cpu.inverse.to(Platform::MPS);
    if (need_counts) out.counts = cpu.counts.to(Platform::MPS);
    return out;
  }

  const std::int64_t nd = a.dim();
  PLUSS_CHECK(nd > 0, "unique(dim): expected rank >= 1");
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "unique(dim): dim out of range");

  Series x = a.contiguous();
  if (dim != 0) x = x.transpose(0, dim).contiguous();

  const std::int64_t rows = x.size(0);
  if (rows == 0) {
    UniqueDimResult_ out{};
    out.values = (dim == 0) ? x : x.transpose(0, dim);
    if (need_inverse) out.inverse = Series::empty({0}, ScalarType::Int64, Platform::CPU);
    if (need_counts) out.counts = Series::empty({0}, ScalarType::Int64, Platform::CPU);
    return out;
  }

  const std::int64_t row_elems = x.numel() / rows;
  const std::size_t row_bytes = static_cast<std::size_t>(row_elems) * x.itemsize();
  const auto* raw = reinterpret_cast<const std::uint8_t*>(x.raw_data());
  const auto row_ptr = [&](std::int64_t i) -> const std::uint8_t* {
    return raw + static_cast<std::size_t>(i) * row_bytes;
  };
  const auto hash_row_bytes = [&](std::int64_t i) -> std::uint64_t {
    return hash_row_bytes_fast_(row_ptr(i), row_bytes);
  };
  const auto eq_row_bytes = [&](std::int64_t i, std::int64_t j) -> bool {
    return std::memcmp(row_ptr(i), row_ptr(j), row_bytes) == 0;
  };
  const auto less_row_bytes = [&](std::int64_t i, std::int64_t j) -> bool {
    return std::memcmp(row_ptr(i), row_ptr(j), row_bytes) < 0;
  };

  std::vector<std::int64_t> keep{};
  keep.reserve(static_cast<std::size_t>(rows));
  std::vector<std::int64_t> inverse{};
  if (need_inverse) inverse.resize(static_cast<std::size_t>(rows), 0);
  std::vector<std::int64_t> counts{};
  if (need_counts) counts.reserve(static_cast<std::size_t>(rows));

  auto dedup_with = [&](auto&& hash_row, auto&& eq_row) {
    std::unordered_map<std::uint64_t, std::vector<std::pair<std::int64_t, std::int64_t>>> buckets;
    buckets.reserve(static_cast<std::size_t>(rows * 2));

    for (std::int64_t i = 0; i < rows; ++i) {
      const std::uint64_t h = hash_row(i);
      auto& chain = buckets[h];
      std::int64_t out_idx = -1;
      for (const auto& rep : chain) {
        if (eq_row(i, rep.first)) {
          out_idx = rep.second;
          break;
        }
      }
      if (out_idx < 0) {
        out_idx = static_cast<std::int64_t>(keep.size());
        chain.push_back({i, out_idx});
        keep.push_back(i);
        if (need_counts) counts.push_back(0);
      }
      if (need_inverse) inverse[static_cast<std::size_t>(i)] = out_idx;
      if (need_counts) counts[static_cast<std::size_t>(out_idx)] += 1;
    }
  };

  auto sort_and_remap = [&](auto&& less_row) {
    if (!sorted) return;
    std::vector<std::int64_t> perm(keep.size());
    for (std::size_t i = 0; i < perm.size(); ++i) perm[i] = static_cast<std::int64_t>(i);
    std::sort(perm.begin(), perm.end(), [&](std::int64_t aidx, std::int64_t bidx) {
      return less_row(keep[static_cast<std::size_t>(aidx)], keep[static_cast<std::size_t>(bidx)]);
    });

    std::vector<std::int64_t> keep_sorted(keep.size());
    std::vector<std::int64_t> old_to_new(keep.size());
    for (std::size_t ni = 0; ni < perm.size(); ++ni) {
      const auto oi = static_cast<std::size_t>(perm[ni]);
      keep_sorted[ni] = keep[oi];
      old_to_new[oi] = static_cast<std::int64_t>(ni);
    }
    keep.swap(keep_sorted);

    if (need_counts) {
      std::vector<std::int64_t> counts_sorted(counts.size());
      for (std::size_t ni = 0; ni < perm.size(); ++ni) {
        counts_sorted[ni] = counts[static_cast<std::size_t>(perm[ni])];
      }
      counts.swap(counts_sorted);
    }
    if (need_inverse) {
      for (auto& v : inverse) v = old_to_new[static_cast<std::size_t>(v)];
    }
  };

  if (x.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(x.raw_data());
    const auto rowf = [&](std::int64_t i) -> const float* { return p + i * row_elems; };
    const auto hash_row = [&](std::int64_t i) -> std::uint64_t {
      const float* r = rowf(i);
      std::uint64_t h = 0xcbf29ce484222325ULL;
      for (std::int64_t k = 0; k < row_elems; ++k) {
        h ^= unique_mix64_(canonical_f32_bits_(r[k]));
        h *= 0x100000001b3ULL;
      }
      return h;
    };
    const auto eq_row = [&](std::int64_t i, std::int64_t j) -> bool {
      const float* ri = rowf(i);
      const float* rj = rowf(j);
      for (std::int64_t k = 0; k < row_elems; ++k) {
        const float u = ri[k];
        const float v = rj[k];
        if (std::isnan(u) && std::isnan(v)) continue;
        if (u != v) return false;
      }
      return true;
    };
    const auto less_row = [&](std::int64_t i, std::int64_t j) -> bool {
      const float* ri = rowf(i);
      const float* rj = rowf(j);
      for (std::int64_t k = 0; k < row_elems; ++k) {
        const float u = ri[k];
        const float v = rj[k];
        const bool un = std::isnan(u);
        const bool vn = std::isnan(v);
        if (un && vn) continue;
        if (un) return false;
        if (vn) return true;
        if (u < v) return true;
        if (u > v) return false;
      }
      return false;
    };
    dedup_with(hash_row, eq_row);
    sort_and_remap(less_row);
  } else if (x.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(x.raw_data());
    const auto rowf = [&](std::int64_t i) -> const double* { return p + i * row_elems; };
    const auto hash_row = [&](std::int64_t i) -> std::uint64_t {
      const double* r = rowf(i);
      std::uint64_t h = 0xcbf29ce484222325ULL;
      for (std::int64_t k = 0; k < row_elems; ++k) {
        h ^= unique_mix64_(canonical_f64_bits_(r[k]));
        h *= 0x100000001b3ULL;
      }
      return h;
    };
    const auto eq_row = [&](std::int64_t i, std::int64_t j) -> bool {
      const double* ri = rowf(i);
      const double* rj = rowf(j);
      for (std::int64_t k = 0; k < row_elems; ++k) {
        const double u = ri[k];
        const double v = rj[k];
        if (std::isnan(u) && std::isnan(v)) continue;
        if (u != v) return false;
      }
      return true;
    };
    const auto less_row = [&](std::int64_t i, std::int64_t j) -> bool {
      const double* ri = rowf(i);
      const double* rj = rowf(j);
      for (std::int64_t k = 0; k < row_elems; ++k) {
        const double u = ri[k];
        const double v = rj[k];
        const bool un = std::isnan(u);
        const bool vn = std::isnan(v);
        if (un && vn) continue;
        if (un) return false;
        if (vn) return true;
        if (u < v) return true;
        if (u > v) return false;
      }
      return false;
    };
    dedup_with(hash_row, eq_row);
    sort_and_remap(less_row);
  } else {
    dedup_with(hash_row_bytes, eq_row_bytes);
    sort_and_remap(less_row_bytes);
  }

  const std::array<std::int64_t, 1> is{{static_cast<std::int64_t>(keep.size())}};
  Series idx = Series::empty(std::span<const std::int64_t>(is.data(), is.size()), ScalarType::Int64, Platform::CPU);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_mutable_data());
  for (std::size_t i = 0; i < keep.size(); ++i) ip[i] = keep[i];

  UniqueDimResult_ out{};
  out.values = x.index_select(0, idx);
  if (dim != 0) out.values = out.values.transpose(0, dim);

  if (need_inverse) {
    const std::array<std::int64_t, 1> inv_shape{{rows}};
    out.inverse = Series::empty(std::span<const std::int64_t>(inv_shape.data(), inv_shape.size()), ScalarType::Int64, Platform::CPU);
    auto* op = reinterpret_cast<std::int64_t*>(out.inverse.raw_mutable_data());
    for (std::size_t i = 0; i < inverse.size(); ++i) op[i] = inverse[i];
  }
  if (need_counts) {
    const std::array<std::int64_t, 1> cnt_shape{{static_cast<std::int64_t>(counts.size())}};
    out.counts = Series::empty(std::span<const std::int64_t>(cnt_shape.data(), cnt_shape.size()), ScalarType::Int64, Platform::CPU);
    auto* op = reinterpret_cast<std::int64_t*>(out.counts.raw_mutable_data());
    for (std::size_t i = 0; i < counts.size(); ++i) op[i] = counts[i];
  }
  return out;
}

} // namespace

Series unique(const Series& a, std::int64_t dim, bool sorted) {
  return unique_dim_impl_(a, dim, sorted, false, false).values;
}

std::vector<Series> unique(const Series& a,
                           std::int64_t dim,
                           bool sorted,
                           bool return_inverse,
                           bool return_counts) {
  UniqueDimResult_ r = unique_dim_impl_(a, dim, sorted, return_inverse, return_counts);
  if (return_inverse && return_counts) return {r.values, r.inverse, r.counts};
  if (return_inverse) return {r.values, r.inverse};
  if (return_counts) return {r.values, r.counts};
  return {r.values};
}

namespace {

Series set_flatten_1d_(const Series& x) {
  return (x.dim() == 1) ? x.contiguous() : x.reshape({x.numel()}).contiguous();
}

template <class T>
Series membership_mask_1d_cpu_hash_(const Series& lhs, const Series& rhs) {
  Series out = Series::empty(lhs.sizes(), ScalarType::Bool, Platform::CPU);
  const T* lp = reinterpret_cast<const T*>(lhs.raw_data());
  const T* rp = reinterpret_cast<const T*>(rhs.raw_data());
  bool* op = reinterpret_cast<bool*>(out.raw_mutable_data());

  std::unordered_set<T> lookup;
  const std::int64_t rhs_n = rhs.numel();
  if (rhs_n > 0) {
    lookup.reserve(static_cast<std::size_t>(rhs_n * 2 + 1));
  }
  for (std::int64_t i = 0; i < rhs_n; ++i) {
    const T v = rp[i];
    if constexpr (std::is_floating_point_v<T>) {
      if (std::isnan(v)) continue;
    }
    lookup.insert(v);
  }

  const std::int64_t lhs_n = lhs.numel();
  for (std::int64_t i = 0; i < lhs_n; ++i) {
    const T v = lp[i];
    if constexpr (std::is_floating_point_v<T>) {
      if (std::isnan(v)) {
        op[i] = false;
        continue;
      }
    }
    op[i] = lookup.find(v) != lookup.end();
  }
  return out;
}

Series membership_mask_1d_(const Series& lhs, const Series& rhs) {
  if (lhs.numel() == 0 || rhs.numel() == 0) {
    return Series::zeros(lhs.sizes(), ScalarType::Bool).to(lhs.platform());
  }
  if (lhs.platform() == Platform::CPU &&
      rhs.platform() == Platform::CPU &&
      lhs.scalar_type() == rhs.scalar_type()) {
    const Series lc = lhs.contiguous();
    const Series rc = rhs.contiguous();
    switch (lc.scalar_type()) {
      case ScalarType::Bool: return membership_mask_1d_cpu_hash_<bool>(lc, rc);
      case ScalarType::Int8: return membership_mask_1d_cpu_hash_<std::int8_t>(lc, rc);
      case ScalarType::UInt8: return membership_mask_1d_cpu_hash_<std::uint8_t>(lc, rc);
      case ScalarType::Int16: return membership_mask_1d_cpu_hash_<std::int16_t>(lc, rc);
      case ScalarType::UInt16: return membership_mask_1d_cpu_hash_<std::uint16_t>(lc, rc);
      case ScalarType::Int32: return membership_mask_1d_cpu_hash_<std::int32_t>(lc, rc);
      case ScalarType::UInt32: return membership_mask_1d_cpu_hash_<std::uint32_t>(lc, rc);
      case ScalarType::Int64: return membership_mask_1d_cpu_hash_<std::int64_t>(lc, rc);
      case ScalarType::Float32: return membership_mask_1d_cpu_hash_<float>(lc, rc);
      case ScalarType::Float64: return membership_mask_1d_cpu_hash_<double>(lc, rc);
      default: break;
    }
  }
  return lhs.unsqueeze(1).eq(rhs.unsqueeze(0)).any(/*dim=*/1, /*keepdim=*/false);
}

Series unique_for_set_1d_(const Series& x, bool assume_unique) {
  if (assume_unique) return set_flatten_1d_(x);
  return unique(set_flatten_1d_(x), /*sorted=*/false);
}

std::int64_t normalize_set_dim_(const Series& x, std::int64_t dim, std::string_view op) {
  const std::int64_t nd = x.dim();
  PLUSS_CHECK(nd > 0, std::string(op) + ": expected rank >= 1");
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(op) + ": dim out of range");
  return dim;
}

std::int64_t require_set_along_compatible_(
    const Series& a,
    const Series& b,
    std::int64_t dim,
    std::string_view op) {
  require_same_platform(a, b, op);
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(), std::string(op) + ": input dtypes must match");
  PLUSS_CHECK(a.dim() == b.dim(), std::string(op) + ": ranks must match");
  const std::int64_t dim_norm = normalize_set_dim_(a, dim, op);
  for (std::int64_t d = 0; d < a.dim(); ++d) {
    if (d == dim_norm) continue;
    PLUSS_CHECK(
        a.size(d) == b.size(d),
        std::string(op) + ": non-target dimensions must match");
  }
  return dim_norm;
}

Series transpose_dim_front_(const Series& x, std::int64_t dim) {
  return (dim == 0) ? x.contiguous() : x.transpose(0, dim).contiguous();
}

Series flatten_front_slices_(const Series& x_front) {
  const std::int64_t rows = x_front.size(0);
  const std::int64_t cols = (rows == 0) ? 0 : (x_front.numel() / rows);
  return x_front.reshape({rows, cols}).contiguous();
}

Series empty_front_like_with_rows_(const Series& ref_front, std::int64_t rows) {
  std::vector<std::int64_t> out_sizes(ref_front.sizes().begin(), ref_front.sizes().end());
  out_sizes[0] = rows;
  return Series::empty(out_sizes, ref_front.scalar_type(), ref_front.platform());
}

Series restore_front_layout_(const Series& x_front, std::int64_t dim) {
  return (dim == 0) ? x_front : x_front.transpose(0, dim).contiguous();
}

bool number_theory_dtype_supported_(ScalarType dt) {
  switch (dt) {
    case ScalarType::Bool:
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
    case ScalarType::UInt8:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
      return true;
    default:
      return false;
  }
}

bool is_prime_i64_(std::int64_t n) {
  if (n <= 1) return false;
  if (n <= 3) return true;
  if ((n % 2) == 0 || (n % 3) == 0) return false;
  for (std::int64_t d = 5; d <= (n / d); d += 6) {
    if ((n % d) == 0 || (n % (d + 2)) == 0) return false;
  }
  return true;
}

std::vector<std::uint8_t> prime_sieve_upto_(std::int64_t max_n) {
  PLUSS_CHECK(max_n >= 0, "internal: sieve max_n must be >= 0");
  std::vector<std::uint8_t> sieve(static_cast<std::size_t>(max_n + 1), static_cast<std::uint8_t>(1));
  if (max_n >= 0) sieve[0] = 0;
  if (max_n >= 1) sieve[1] = 0;
  for (std::int64_t p = 2; p <= (max_n / p); ++p) {
    if (!sieve[static_cast<std::size_t>(p)]) continue;
    for (std::int64_t m = p * p; m <= max_n; m += p) {
      sieve[static_cast<std::size_t>(m)] = 0;
    }
  }
  return sieve;
}

const std::vector<std::int64_t>& nth_prime_table_upto_(std::int64_t n_max) {
  static thread_local std::vector<std::int64_t> cache;
  if (static_cast<std::int64_t>(cache.size()) >= n_max) {
    return cache;
  }

  auto upper_bound_nth_prime = [](std::int64_t n) -> std::int64_t {
    if (n < 6) return 15;
    const double nd = static_cast<double>(n);
    const double ub = nd * (std::log(nd) + std::log(std::log(nd)));
    return static_cast<std::int64_t>(std::ceil(ub + 16.0));
  };

  std::int64_t upper = upper_bound_nth_prime(n_max);
  while (true) {
    const auto sieve = prime_sieve_upto_(upper);
    std::vector<std::int64_t> primes;
    primes.reserve(static_cast<std::size_t>(n_max));
    for (std::int64_t v = 2; v <= upper; ++v) {
      if (!sieve[static_cast<std::size_t>(v)]) continue;
      primes.push_back(v);
      if (static_cast<std::int64_t>(primes.size()) == n_max) break;
    }
    if (static_cast<std::int64_t>(primes.size()) >= n_max) {
      cache = std::move(primes);
      return cache;
    }
    upper = upper + (upper / 2) + 64;
  }
}

std::int64_t least_prime_factor_i64_(std::int64_t n) {
  if (n == 0) return 0;
  if (n == 1 || n == -1) return 1;
  PLUSS_CHECK(n != std::numeric_limits<std::int64_t>::min(), "factor: value too small to factor safely");
  std::int64_t x = n < 0 ? -n : n;
  if ((x % 2) == 0) return 2;
  if ((x % 3) == 0) return 3;
  for (std::int64_t d = 5; d <= (x / d); d += 6) {
    if ((x % d) == 0) return d;
    const std::int64_t d2 = d + 2;
    if ((x % d2) == 0) return d2;
  }
  return x;
}

Series as_i64_cpu_for_number_theory_(const Series& input, std::string_view op) {
  PLUSS_CHECK(number_theory_dtype_supported_(input.scalar_type()),
              std::string(op) + ": expects integral/bool dtype (UInt64 currently unsupported)");
  Series src = input.contiguous();
  if (src.scalar_type() != ScalarType::Int64) {
    src = src.to(ScalarType::Int64);
  }
  if (src.platform() != Platform::CPU) {
    src = src.to(Platform::CPU);
  }
  return src;
}

Series is_prime_series_fast_(const Series& input) {
  const Series src = as_i64_cpu_for_number_theory_(input, "is_prime");
  Series out = Series::empty(src.sizes(), ScalarType::Bool, Platform::CPU);

  const std::int64_t* ip = reinterpret_cast<const std::int64_t*>(src.raw_data());
  bool* optr = reinterpret_cast<bool*>(out.raw_mutable_data());
  const std::int64_t n = src.numel();
  std::int64_t max_pos = 1;
  for (std::int64_t i = 0; i < n; ++i) {
    if (ip[i] > max_pos) max_pos = ip[i];
  }

  constexpr std::int64_t kSieveLimit = 5000000;
  const bool use_sieve = (max_pos >= 2 && max_pos <= kSieveLimit);
  std::vector<std::uint8_t> sieve;
  if (use_sieve) {
    sieve = prime_sieve_upto_(max_pos);
  }
  std::unordered_map<std::int64_t, bool> cache;
  cache.reserve(static_cast<std::size_t>(std::min<std::int64_t>(n, 1024)));

  for (std::int64_t i = 0; i < n; ++i) {
    const std::int64_t v = ip[i];
    if (v < 2) {
      optr[i] = false;
      continue;
    }
    if (use_sieve && v <= max_pos) {
      optr[i] = sieve[static_cast<std::size_t>(v)] != 0;
      continue;
    }
    const auto it = cache.find(v);
    if (it != cache.end()) {
      optr[i] = it->second;
      continue;
    }
    const bool is_p = is_prime_i64_(v);
    cache.emplace(v, is_p);
    optr[i] = is_p;
  }

  if (input.platform() != Platform::CPU) {
    out = out.to(input.platform());
  }
  return out;
}

Series nth_prime_series_fast_(const Series& input) {
  const Series src = as_i64_cpu_for_number_theory_(input, "prime");
  const std::int64_t* ip = reinterpret_cast<const std::int64_t*>(src.raw_data());
  const std::int64_t n = src.numel();

  std::int64_t max_n = 1;
  for (std::int64_t i = 0; i < n; ++i) {
    const std::int64_t v = ip[i];
    PLUSS_CHECK(v >= 1, "prime: expects n >= 1");
    PLUSS_CHECK(v <= 200000, "prime: n too large for direct nth-prime routine");
    if (v > max_n) max_n = v;
  }
  const auto& table = nth_prime_table_upto_(max_n);

  Series out = Series::empty(src.sizes(), ScalarType::Int64, Platform::CPU);
  std::int64_t* optr = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  for (std::int64_t i = 0; i < n; ++i) {
    optr[i] = table[static_cast<std::size_t>(ip[i] - 1)];
  }
  if (input.platform() != Platform::CPU) {
    out = out.to(input.platform());
  }
  return out;
}

Series least_prime_factor_series_fast_(const Series& input) {
  const Series src = as_i64_cpu_for_number_theory_(input, "factor");
  const std::int64_t* ip = reinterpret_cast<const std::int64_t*>(src.raw_data());
  const std::int64_t n = src.numel();

  std::int64_t max_abs = 0;
  for (std::int64_t i = 0; i < n; ++i) {
    const std::int64_t v = ip[i];
    PLUSS_CHECK(v != std::numeric_limits<std::int64_t>::min(), "factor: value too small to factor safely");
    const std::int64_t av = (v < 0) ? -v : v;
    if (av > max_abs) max_abs = av;
  }

  Series out = Series::empty(src.sizes(), ScalarType::Int64, Platform::CPU);
  std::int64_t* optr = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());

  constexpr std::int64_t kSpfLimit = 5000000;
  if (max_abs <= kSpfLimit) {
    std::vector<std::int32_t> spf(static_cast<std::size_t>(max_abs + 1), 0);
    for (std::int64_t i = 2; i <= max_abs; ++i) {
      if (spf[static_cast<std::size_t>(i)] != 0) continue;
      spf[static_cast<std::size_t>(i)] = static_cast<std::int32_t>(i);
      if (i > (max_abs / i)) continue;
      for (std::int64_t j = i * i; j <= max_abs; j += i) {
        if (spf[static_cast<std::size_t>(j)] == 0) {
          spf[static_cast<std::size_t>(j)] = static_cast<std::int32_t>(i);
        }
      }
    }
    for (std::int64_t i = 0; i < n; ++i) {
      const std::int64_t v = ip[i];
      if (v == 0) {
        optr[i] = 0;
        continue;
      }
      if (v == 1 || v == -1) {
        optr[i] = 1;
        continue;
      }
      const std::int64_t x = (v < 0) ? -v : v;
      const std::int32_t f = spf[static_cast<std::size_t>(x)];
      optr[i] = (f == 0) ? x : static_cast<std::int64_t>(f);
    }
  } else {
    std::unordered_map<std::int64_t, std::int64_t> cache;
    cache.reserve(static_cast<std::size_t>(std::min<std::int64_t>(n, 1024)));
    for (std::int64_t i = 0; i < n; ++i) {
      const std::int64_t v = ip[i];
      const auto it = cache.find(v);
      if (it != cache.end()) {
        optr[i] = it->second;
        continue;
      }
      const std::int64_t f = least_prime_factor_i64_(v);
      cache.emplace(v, f);
      optr[i] = f;
    }
  }

  if (input.platform() != Platform::CPU) {
    out = out.to(input.platform());
  }
  return out;
}

Series cartprod_pair_1d_(const Series& lhs_raw, const Series& rhs_raw) {
  require_same_platform(lhs_raw, rhs_raw, "cartprod");
  PLUSS_CHECK(lhs_raw.scalar_type() == rhs_raw.scalar_type(), "cartprod: input dtypes must match");
  const Series lhs = set_flatten_1d_(lhs_raw);
  const Series rhs = set_flatten_1d_(rhs_raw);
  const std::int64_t n = lhs.numel();
  const std::int64_t m = rhs.numel();
  if (n == 0 || m == 0) {
    return Series::empty(std::vector<std::int64_t>{0, 2}, lhs.scalar_type(), lhs.platform());
  }
  const Series left = lhs.repeat_interleave(m, /*dim=*/0).reshape({n * m, 1});
  const Series right = rhs.repeat(std::vector<std::int64_t>{n}).reshape({n * m, 1});
  return pluss::unite({left, right}, /*dim=*/1, /*strict_dtype=*/true);
}

} // namespace

Series isin(const Series& elements, const Series& test_elements) {
  return isin(elements, test_elements, /*assume_unique=*/false, /*invert=*/false);
}

Series isin(const Series& elements, const Series& test_elements, bool assume_unique) {
  return isin(elements, test_elements, assume_unique, /*invert=*/false);
}

Series isin(const Series& elements,
            const Series& test_elements,
            bool assume_unique,
            bool invert) {
  require_same_platform(elements, test_elements, "isin");
  PLUSS_CHECK(
      elements.scalar_type() == test_elements.scalar_type(),
      "isin: elements and test_elements must have the same dtype");

  const Series elems = set_flatten_1d_(elements);
  Series tests = set_flatten_1d_(test_elements);
  if (!assume_unique) {
    tests = unique(tests, /*sorted=*/false);
  }

  Series mask = membership_mask_1d_(elems, tests);
  if (invert) {
    mask = mask.logical_not();
  }
  if (elements.dim() == 1) {
    return mask;
  }
  return mask.reshape(elements.sizes());
}

Series union1d(const Series& a, const Series& b) {
  return union1d(a, b, /*sorted=*/true);
}

Series union1d(const Series& a, const Series& b, bool sorted) {
  require_same_platform(a, b, "union1d");
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(), "union1d: input dtypes must match");

  const Series joined = pluss::unite(
      {set_flatten_1d_(a), set_flatten_1d_(b)},
      /*dim=*/0,
      /*strict_dtype=*/true);
  return unique(joined, sorted);
}

Series intersect1d(const Series& a, const Series& b) {
  return intersect1d(a, b, /*sorted=*/true, /*assume_unique=*/false);
}

Series intersect1d(const Series& a, const Series& b, bool sorted) {
  return intersect1d(a, b, sorted, /*assume_unique=*/false);
}

Series intersect1d(const Series& a,
                   const Series& b,
                   bool sorted,
                   bool assume_unique) {
  require_same_platform(a, b, "intersect1d");
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(), "intersect1d: input dtypes must match");

  const Series ua = unique_for_set_1d_(a, assume_unique);
  const Series ub = unique_for_set_1d_(b, assume_unique);
  const Series mask = membership_mask_1d_(ua, ub);
  Series out = ua.masked_select(mask);
  if (sorted) {
    return unique(out, /*sorted=*/true);
  }
  return out;
}

Series setdiff1d(const Series& a, const Series& b) {
  return setdiff1d(a, b, /*sorted=*/true, /*assume_unique=*/false);
}

Series setdiff1d(const Series& a, const Series& b, bool sorted) {
  return setdiff1d(a, b, sorted, /*assume_unique=*/false);
}

Series setdiff1d(const Series& a,
                 const Series& b,
                 bool sorted,
                 bool assume_unique) {
  require_same_platform(a, b, "setdiff1d");
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(), "setdiff1d: input dtypes must match");

  const Series ua = unique_for_set_1d_(a, assume_unique);
  const Series ub = unique_for_set_1d_(b, assume_unique);
  const Series mask = membership_mask_1d_(ua, ub).logical_not();
  Series out = ua.masked_select(mask);
  if (sorted) {
    return unique(out, /*sorted=*/true);
  }
  return out;
}

Series setxor1d(const Series& a, const Series& b) {
  return setxor1d(a, b, /*sorted=*/true, /*assume_unique=*/false);
}

Series setxor1d(const Series& a, const Series& b, bool sorted) {
  return setxor1d(a, b, sorted, /*assume_unique=*/false);
}

Series setxor1d(const Series& a,
                const Series& b,
                bool sorted,
                bool assume_unique) {
  require_same_platform(a, b, "setxor1d");
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(), "setxor1d: input dtypes must match");

  const Series ua = unique_for_set_1d_(a, assume_unique);
  const Series ub = unique_for_set_1d_(b, assume_unique);
  const Series only_a = setdiff1d(ua, ub, /*sorted=*/false, /*assume_unique=*/true);
  const Series only_b = setdiff1d(ub, ua, /*sorted=*/false, /*assume_unique=*/true);
  Series out = pluss::unite({only_a, only_b}, /*dim=*/0, /*strict_dtype=*/true);
  if (sorted) {
    return unique(out, /*sorted=*/true);
  }
  return out;
}

Series set_unionize(const Series& a,
                    pluss::msharp::RangeView::Of<Series> others,
                    bool sorted) {
  if (others.empty()) {
    return unique(set_flatten_1d_(a), sorted);
  }
  if (others.size() == 1) {
    return union1d(a, *others.begin(), sorted);
  }

  Series out = set_flatten_1d_(a);
  for (const Series& other : others) {
    out = union1d(out, other, /*sorted=*/false);
  }
  if (sorted) {
    return unique(out, /*sorted=*/true);
  }
  return out;
}

Series set_intersection(const Series& a,
                        pluss::msharp::RangeView::Of<Series> others,
                        bool sorted,
                        bool assume_unique) {
  if (others.empty()) {
    return unique(unique_for_set_1d_(a, assume_unique), sorted);
  }
  if (others.size() == 1) {
    return intersect1d(a, *others.begin(), sorted, assume_unique);
  }

  Series out = unique_for_set_1d_(a, assume_unique);
  for (const Series& other : others) {
    const Series rhs = unique_for_set_1d_(other, assume_unique);
    out = intersect1d(out, rhs, /*sorted=*/false, /*assume_unique=*/true);
    if (out.numel() == 0) {
      break;
    }
  }
  if (sorted) {
    return unique(out, /*sorted=*/true);
  }
  return out;
}

Series unionize_along(const Series& a,
                      const Series& b,
                      std::int64_t dim,
                      bool sorted) {
  const std::int64_t dim_norm = require_set_along_compatible_(a, b, dim, "unionize_along");
  const Series joined = pluss::unite({a, b}, dim_norm, /*strict_dtype=*/true);
  return unique(joined, dim_norm, sorted);
}

Series intersection_along(const Series& a,
                          const Series& b,
                          std::int64_t dim,
                          bool sorted,
                          bool assume_unique) {
  const std::int64_t dim_norm = require_set_along_compatible_(a, b, dim, "intersection_along");
  const Series a_front = transpose_dim_front_(a, dim_norm);
  const Series b_front = transpose_dim_front_(b, dim_norm);
  const Series ua = assume_unique ? a_front : unique(a_front, /*dim=*/0, /*sorted=*/false);
  const Series ub = assume_unique ? b_front : unique(b_front, /*dim=*/0, /*sorted=*/false);

  Series out_front = empty_front_like_with_rows_(ua, 0);
  if (ua.size(0) != 0 && ub.size(0) != 0) {
    const Series ua_rows = flatten_front_slices_(ua);
    const Series ub_rows = flatten_front_slices_(ub);
    const Series mask = ua_rows.unsqueeze(1).eq(ub_rows.unsqueeze(0)).all(/*dim=*/2, /*keepdim=*/false).any(/*dim=*/1, /*keepdim=*/false);
    const Series idx2d = nonzero(mask);
    if (idx2d.numel() > 0) {
      const Series idx = idx2d.squeeze(-1);
      out_front = ua.index_select(0, idx);
    }
  }

  if (sorted) {
    out_front = unique(out_front, /*dim=*/0, /*sorted=*/true);
  }
  return restore_front_layout_(out_front, dim_norm);
}

Series setdiff_along(const Series& a,
                     const Series& b,
                     std::int64_t dim,
                     bool sorted,
                     bool assume_unique) {
  const std::int64_t dim_norm = require_set_along_compatible_(a, b, dim, "setdiff_along");
  const Series a_front = transpose_dim_front_(a, dim_norm);
  const Series b_front = transpose_dim_front_(b, dim_norm);
  const Series ua = assume_unique ? a_front : unique(a_front, /*dim=*/0, /*sorted=*/false);
  const Series ub = assume_unique ? b_front : unique(b_front, /*dim=*/0, /*sorted=*/false);

  Series out_front = empty_front_like_with_rows_(ua, 0);
  if (ua.size(0) != 0) {
    Series keep_mask = Series::ones({ua.size(0)}, ScalarType::Bool).to(ua.platform());
    if (ub.size(0) != 0) {
      const Series ua_rows = flatten_front_slices_(ua);
      const Series ub_rows = flatten_front_slices_(ub);
      keep_mask =
          ua_rows.unsqueeze(1)
              .eq(ub_rows.unsqueeze(0))
              .all(/*dim=*/2, /*keepdim=*/false)
              .any(/*dim=*/1, /*keepdim=*/false)
              .logical_not();
    }
    const Series idx2d = nonzero(keep_mask);
    if (idx2d.numel() > 0) {
      const Series idx = idx2d.squeeze(-1);
      out_front = ua.index_select(0, idx);
    }
  }

  if (sorted) {
    out_front = unique(out_front, /*dim=*/0, /*sorted=*/true);
  }
  return restore_front_layout_(out_front, dim_norm);
}

Series setxor_along(const Series& a,
                    const Series& b,
                    std::int64_t dim,
                    bool sorted,
                    bool assume_unique) {
  const std::int64_t dim_norm = require_set_along_compatible_(a, b, dim, "setxor_along");
  const Series only_a = setdiff_along(a, b, dim_norm, /*sorted=*/false, assume_unique);
  const Series only_b = setdiff_along(b, a, dim_norm, /*sorted=*/false, assume_unique);
  return unionize_along(only_a, only_b, dim_norm, sorted);
}

Series bincount(const Series& a, std::int64_t minlength) {
  PLUSS_CHECK(minlength >= 0, "bincount: minlength must be >= 0");

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_counting_enabled();
    const bool native_strict = native::cuda::native_counting_strict();
    const bool route_supported = native::cuda::supports_bincount_counting(a, minlength);
    return cuda_counting_route_or_bridge_(
        "bincount",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series cpu = a.to(Platform::CPU);
          return bincount(cpu, minlength).to(Platform::CUDA);
        });
  }

  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(
        a.scalar_type() == ScalarType::Int64 || a.scalar_type() == ScalarType::Int32,
        "bincount: MPS path currently supports int32/int64 input only");
    auto x = (a.dim() == 1) ? a.contiguous() : a.reshape({a.numel()}).contiguous();
    if (x.numel() == 0) {
      const auto outn = std::max<std::int64_t>(0, minlength);
      return Series::zeros({outn}, ScalarType::Int64).to(Platform::MPS);
    }

    const Series min_s = amin(x).to(Platform::CPU).contiguous();
    const Series max_s = amax(x).to(Platform::CPU).contiguous();
    PLUSS_CHECK(min_s.numel() == 1 && max_s.numel() == 1, "bincount: internal scalar extraction failure");
    const auto min_v = read_i64_element(min_s, 0);
    const auto maxv = read_i64_element(max_s, 0);
    PLUSS_CHECK(min_v >= 0, "bincount: input must be non-negative");
    const auto outn = std::max<std::int64_t>(minlength, maxv + 1);

    auto src = Series::ones({x.numel()}, ScalarType::Float32).to(Platform::MPS);
    auto out = Series::zeros({outn}, ScalarType::Float32).to(Platform::MPS);
    out = scatter_add(out, 0, x, src);
    return out.round().to(ScalarType::Int64);
  }

  PLUSS_CHECK(a.scalar_type() == ScalarType::Int64 || a.scalar_type() == ScalarType::Int32 ||
              a.scalar_type() == ScalarType::Int16 || a.scalar_type() == ScalarType::Int8, "bincount: input must be integer");
  auto x = a.contiguous();
  const auto n = x.numel();
  std::int64_t maxv = -1;
  for (std::int64_t i=0;i<n;++i) {
    const auto v = read_i64_element(x, i);
    PLUSS_CHECK(v >= 0, "bincount: input must be non-negative");
    if (v > maxv) maxv = v;
  }
  const std::int64_t outn = std::max<std::int64_t>(minlength, maxv + 1);
  Series out = Series::zeros({outn}, ScalarType::Int64).to(x.platform());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  for (std::int64_t i=0;i<n;++i) {
    const auto v = read_i64_element(x, i);
    ++op[v];
  }
  return out;
}

Series histc(const Series& a, std::int64_t bins, double min, double max) {
  PLUSS_CHECK(bins > 0, "histc: bins must be > 0");

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_counting_enabled();
    const bool native_strict = native::cuda::native_counting_strict();
    const bool route_supported = native::cuda::supports_histc_counting(a, bins);
    return cuda_counting_route_or_bridge_(
        "histc",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series cpu = a.to(Platform::CPU);
          return histc(cpu, bins, min, max).to(Platform::CUDA);
        });
  }

  if (a.platform() == Platform::MPS) {
    auto x = ((a.dim() == 1) ? a.contiguous() : a.reshape({a.numel()}).contiguous()).to(ScalarType::Float32);
    const auto n = x.numel();

    if (min == 0.0 && max == 0.0) {
      auto mn_s = amin(x);
      auto mx_s = amax(x);
      const auto mn_h = mn_s.host<float>().array();
      const auto mx_h = mx_s.host<float>().array();
      PLUSS_CHECK(!mn_h.data.empty() && !mx_h.data.empty(), "histc: internal scalar extraction failure");
      min = static_cast<double>(mn_h.data[0]);
      max = static_cast<double>(mx_h.data[0]);
    }

    const auto min_s = scalar_like_(x, min);
    const auto max_s = scalar_like_(x, max);
    auto valid = isfinite(x).logical_and(ge(x, min_s)).logical_and(le(x, max_s));
    auto weights = valid.to(ScalarType::Float32);

    auto out = Series::zeros({bins}, ScalarType::Float32).to(Platform::MPS);
    if (n == 0) {
      return out.to(ScalarType::Int64);
    }

    auto idx_f = Series::zeros({n}, ScalarType::Float32).to(Platform::MPS);
    if (!(max > min) || bins == 1) {
      idx_f = Series::zeros({n}, ScalarType::Float32).to(Platform::MPS);
    } else {
      const auto scale_s = scalar_like_(x, static_cast<double>(bins) / (max - min));
      idx_f = floor(mul(sub(x, min_s), scale_s));
      const auto last_bin = scalar_like_(x, static_cast<double>(bins - 1));
      idx_f = where(eq(x, max_s), last_bin, idx_f);
      idx_f = clamp_max(clamp_min(idx_f, 0.0), static_cast<double>(bins - 1));
    }
    auto idx = idx_f.to(ScalarType::Int64);
    out = scatter_add(out, 0, idx, weights);
    return out.to(ScalarType::Int64);
  }

  const bool use_f32 = (a.platform() == Platform::MPS);
  auto x = a.contiguous().to(use_f32 ? ScalarType::Float32 : ScalarType::Float64);
  const auto n = x.numel();
  if (min == 0.0 && max == 0.0) {
    // infer min/max ignoring NaN/inf
    bool any = false;
    double mn=0, mx=0;
    for (std::int64_t i=0;i<n;++i) {
      const double v = use_f32
                           ? static_cast<double>(reinterpret_cast<const float*>(x.raw_data())[i])
                           : reinterpret_cast<const double*>(x.raw_data())[i];
      if (!std::isfinite(v)) continue;
      if (!any) { mn = mx = v; any=true; }
      else { mn = std::min(mn,v); mx = std::max(mx,v); }
    }
    if (!any) { mn = 0.0; mx = 0.0; }
    min = mn; max = mx;
  }
  Series out = Series::zeros({bins}, ScalarType::Int64).to(x.platform());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  if (bins == 1) {
    // all finite in one bin if within [min,max]
    for (std::int64_t i=0;i<n;++i) {
      const double v = use_f32
                           ? static_cast<double>(reinterpret_cast<const float*>(x.raw_data())[i])
                           : reinterpret_cast<const double*>(x.raw_data())[i];
      if (!std::isfinite(v)) continue;
      if (v >= min && v <= max) ++op[0];
    }
    return out;
  }
  const double scale = (bins) / (max - min);
  for (std::int64_t i=0;i<n;++i) {
    const double v = use_f32
                         ? static_cast<double>(reinterpret_cast<const float*>(x.raw_data())[i])
                         : reinterpret_cast<const double*>(x.raw_data())[i];
    if (!std::isfinite(v)) continue;
    if (v < min || v > max) continue;
    std::int64_t b = (v == max) ? (bins-1) : (std::int64_t)((v - min) * scale);
    if (b < 0) b = 0;
    if (b >= bins) b = bins-1;
    ++op[b];
  }
  return out;
}

Series nonzero(const Series& a) {
  if (a.numel() == 0) {
    const std::array<std::int64_t, 2> out_shape{{0, a.dim()}};
    return Series::empty(std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
                         ScalarType::Int64,
                         a.platform());
  }

  const bool scalar_like = (a.dim() == 0) || (a.dim() == 1 && a.numel() == 1);
  if (scalar_like) {
    const bool present = a.ne(0.0).to(Platform::CPU).to_vec<bool>(0);
    const std::array<std::int64_t, 2> out_shape{{present ? 1 : 0, 0}};
    return Series::empty(std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
                         ScalarType::Int64,
                         a.platform());
  }

  if (a.platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const bool route_supported = native::mps::supports_nonzero(ScalarType::Bool);
    if (strict_native && !route_supported) {
      require_mps_native_dispatch_supported_("nonzero");
    }
    if (route_supported) {
      const Series mask = a.ne(0.0).contiguous();
      return native::mps::nonzero(mask);
    }
    const Series cpu = a.to(Platform::CPU);
    return nonzero(cpu).to(Platform::MPS);
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_indexing_enabled();
    const bool native_strict = native::cuda::native_indexing_strict();
    const bool route_supported = native::cuda::supports_nonzero(a);
    return cuda_indexing_route_or_bridge_(
        "nonzero",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          const Series cpu = a.to(Platform::CPU);
          return nonzero(cpu).to(Platform::CUDA);
        });
  }

  // Torch returns a 2D int64 tensor of indices (nnz x ndim). For scalar, returns either (1 x 0)??;
  // We'll follow: if scalar and nonzero -> (1 x 0), else (0 x 0).
  const auto nd = a.dim();
  auto mask = (nd == 0) ? a.ne(0.0) : a.ne(0.0).contiguous(); // bool mask
  const bool* mp = reinterpret_cast<const bool*>(mask.raw_data());
  const auto n = mask.numel();
  std::int64_t count = 0;
  for (std::int64_t i=0;i<n;++i) if (mp[i]) ++count;

  std::vector<std::int64_t> out_sizes;
  if (nd == 0) out_sizes = {count, 0};
  else out_sizes = {count, nd};

  Series out = Series::empty(out_sizes, ScalarType::Int64, a.platform());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());

  if (count == 0) return out;
  if (nd == 0) return out;

  // Precompute strides for row-major index unraveling
  std::vector<std::int64_t> strides((std::size_t)nd, 1);
  for (std::int64_t d = nd-2; d >= 0; --d) {
    strides[(std::size_t)d] = strides[(std::size_t)(d+1)] * a.sizes()[(std::size_t)(d+1)];
  }

  std::int64_t row = 0;
  for (std::int64_t lin=0; lin<n; ++lin) {
    if (!mp[lin]) continue;
    std::int64_t rem = lin;
    for (std::int64_t d=0; d<nd; ++d) {
      const auto s = strides[(std::size_t)d];
      const auto idx = rem / s;
      rem -= idx * s;
      op[row*nd + d] = idx;
    }
    ++row;
  }
  return out;
}

static bool same_numel(std::span<const std::int64_t> a, std::span<const std::int64_t> b) {
  return compute_numel(a) == compute_numel(b);
}

static bool can_view_as(const Series& x, std::span<const std::int64_t> new_sizes) {
  // Conservative v0: only contiguous may view.
  return x.is_contiguous() && same_numel(x.sizes(), new_sizes);
}

Series mps_cat_along_dim_(const std::vector<Series>& xs_in, std::int64_t dim, std::string_view op_name) {
  PLUSS_CHECK(!xs_in.empty(), std::string(op_name) + ": expected at least one tensor");
  const ScalarType target_dt = xs_in.front().scalar_type();
  const bool supports_direct =
      native::mps::supports_scatter(target_dt, ScalarType::Int64, target_dt);
  const bool supports_f32 =
      native::mps::supports_scatter(ScalarType::Float32, ScalarType::Int64, ScalarType::Float32);
  const bool use_f32_lane = !supports_direct && mps_exact_f32_roundtrip_lane_dtype_(target_dt) && supports_f32;
  require_mps_support_or_policy_(
      supports_direct || use_f32_lane,
      std::string(op_name));

  const auto nd = xs_in.front().dim();
  PLUSS_CHECK(nd >= 1, std::string(op_name) + ": scalar tensors are not supported");

  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(op_name) + ": dim out of range");

  std::vector<Series> xs;
  xs.reserve(xs_in.size());
  for (const auto& t : xs_in) {
    PLUSS_CHECK(t.defined(), std::string(op_name) + ": undefined tensor");
    PLUSS_CHECK(t.platform() == Platform::MPS, std::string(op_name) + ": expected MPS tensors");
    PLUSS_CHECK(t.scalar_type() == target_dt,
                std::string(op_name) + ": dtype mismatch across inputs");
    PLUSS_CHECK(t.dim() == nd, std::string(op_name) + ": rank mismatch");
    xs.push_back(t.is_contiguous() ? t : t.contiguous());
  }

  std::vector<std::int64_t> out_sizes(xs.front().sizes().begin(), xs.front().sizes().end());
  std::int64_t cat_len = 0;
  for (const auto& t : xs) {
    for (std::int64_t d = 0; d < nd; ++d) {
      if (d == dim) continue;
      PLUSS_CHECK(
          t.sizes()[static_cast<std::size_t>(d)] == out_sizes[static_cast<std::size_t>(d)],
          std::string(op_name) + ": non-concatenation dimensions must match");
    }
    cat_len += t.sizes()[static_cast<std::size_t>(dim)];
  }
  out_sizes[static_cast<std::size_t>(dim)] = cat_len;

  const ScalarType work_dt = supports_direct ? target_dt : ScalarType::Float32;
  Series out = Series::empty(out_sizes, work_dt, Platform::MPS);
  std::int64_t offset = 0;
  for (const auto& t : xs) {
    const auto& ts = t.sizes();
    const std::int64_t dlen = ts[static_cast<std::size_t>(dim)];
    std::int64_t outer = 1;
    std::int64_t inner = 1;
    for (std::int64_t d = 0; d < dim; ++d) outer *= ts[static_cast<std::size_t>(d)];
    for (std::int64_t d = dim + 1; d < nd; ++d) inner *= ts[static_cast<std::size_t>(d)];

    Series idx_cpu = Series::empty(ts, ScalarType::Int64, Platform::CPU);
    auto* ip = reinterpret_cast<std::int64_t*>(idx_cpu.raw_mutable_data());
    for (std::int64_t o = 0; o < outer; ++o) {
      const std::int64_t base_o = o * dlen * inner;
      for (std::int64_t k = 0; k < dlen; ++k) {
        const std::int64_t dst = offset + k;
        const std::int64_t base_ok = base_o + k * inner;
        for (std::int64_t i = 0; i < inner; ++i) {
          ip[base_ok + i] = dst;
        }
      }
    }
    const Series idx = idx_cpu.to(Platform::MPS);
    const Series src = (work_dt == target_dt) ? t : t.to(work_dt);
    native::mps::scatter_set_(out, dim, idx, src);
    offset += dlen;
  }
  if (work_dt != target_dt) {
    out = out.to(target_dt);
  }
  return out;
}

} // namespace pluss::codem

namespace pluss::codem {

using namespace ops_internal;

Series Series::undefined() {
  return Series();
}

Series Series::empty(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  return empty(sizes, st, scoped_platform_or_cpu_());
}

Series Series::empty(pluss::msharp::RangeView::Int sizes, ScalarType st, Platform platform) {
  if (platform == Platform::CUDA) {
    if (!native::cuda::runtime_available()) {
      PLUSS_FAIL(
          std::string("Series::empty(..., CUDA): CUDA runtime unavailable: ") +
          native::cuda::runtime_error_cstr());
    }
  }
  if (platform == Platform::MPS) {
    if (!native::mps::runtime_available()) {
      PLUSS_FAIL(
          std::string("Series::empty(..., MPS): MPS runtime unavailable: ") +
          native::mps::runtime_error_cstr());
    }
    PLUSS_CHECK(st != ScalarType::Float64,
                "Series::empty(..., MPS): float64 is unsupported on MPS; use float32/float16/bfloat16");
  }
  const auto dt = dtype_of(st);
  const auto n = compute_numel(sizes);
  const auto bytes = std::max<std::size_t>(
      1, static_cast<std::size_t>(n) * dt.itemsize);
  Storage storage(bytes);
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      std::move(storage), 0,
      std::vector<std::int64_t>(sizes.begin(), sizes.end()),
      contiguous_strides(sizes),
      st,
      platform);
  return SeriesFactoryAccess::wrap(std::move(impl));
}


Series Series::zeros(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  auto t = empty(sizes, st);
  const auto dt = t.dtype();
  std::memset(t.raw_data(), 0, static_cast<std::size_t>(t.numel()) * dt.itemsize);
  return t;
}

Series Series::ones(pluss::msharp::RangeView::Int sizes, ScalarType st) {
  auto t = empty(sizes, st);
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

Series Series::full(pluss::msharp::RangeView::Int sizes, double value, ScalarType st) {
  auto t = empty(sizes, st);
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
      Series tmp = Series::full(sizes, value, ScalarType::Float32);
      auto half = tmp.to(ScalarType::Float16);
      std::memcpy(p, half.raw_data(), (std::size_t)n * 2);
      return t;
    }
    case ScalarType::BFloat16: {
      auto* p = reinterpret_cast<std::uint16_t*>(t.raw_data());
      Series tmp = Series::full(sizes, value, ScalarType::Float32);
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

Series Series::view(pluss::msharp::RangeView::Int new_sizes) const {
  if (!defined()) PLUSS_FAIL("view: undefined");
  if (!can_view_as(*this, new_sizes)) PLUSS_FAIL("view: requires contiguous in v0");
  sync_mps_alias_source_if_needed_(*this);
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::vector<std::int64_t>(new_sizes.begin(), new_sizes.end()),
      contiguous_strides(new_sizes),
      scalar_type(),
      platform());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  maybe_link_unary_op_(
      out,
      *this,
      [shape = std::vector<std::int64_t>(sizes().begin(), sizes().end())](const Series& out_sens) {
        return out_sens.reshape(shape);
      });
  maybe_link_alias_lineage_(out, *this, "view");
  return out;
}

Series Series::reshape(pluss::msharp::RangeView::Int new_sizes) const {
  if (!defined()) PLUSS_FAIL("reshape: undefined");
  if (can_view_as(*this, new_sizes)) return view(new_sizes);
  Series c = contiguous();
  // Preserve reverse-mode connectivity through the contiguous staging copy
  // used by reshape fallback for non-contiguous inputs.
  maybe_link_unary_op_(c, *this, "reshape_contiguous", [](const Series& g) { return g; });
  return c.view(new_sizes);
}

Series Series::transpose(std::int64_t d0, std::int64_t d1) const {
  if (!defined()) PLUSS_FAIL("transpose: undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = dim();
  if (d0 < 0) d0 += nd;
  if (d1 < 0) d1 += nd;
  if (d0 < 0 || d0 >= nd || d1 < 0 || d1 >= nd) PLUSS_FAIL("transpose: dim out of range");
  if (d0 == d1) return *this;

  auto ns = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  auto nt = std::vector<std::int64_t>(strides().begin(), strides().end());
  std::swap(ns[static_cast<std::size_t>(d0)], ns[static_cast<std::size_t>(d1)]);
  std::swap(nt[static_cast<std::size_t>(d0)], nt[static_cast<std::size_t>(d1)]);

  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::move(ns), std::move(nt),
      scalar_type(),
      platform());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  maybe_link_unary_op_(
      out,
      *this,
      [d0, d1](const Series& out_sens) {
        return out_sens.transpose(d0, d1);
      });
  maybe_link_alias_lineage_(out, *this, "transpose");
  return out;
}


Series Series::permute(pluss::msharp::RangeView::Int dims) const {
  if (!defined()) PLUSS_FAIL("permute: undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = dim();
  PLUSS_CHECK((std::int64_t)dims.size() == nd, "permute: dims must have length == dim()");
  std::vector<std::int64_t> dd(dims.begin(), dims.end());
  // normalize negatives
  for (auto& d : dd) { if (d < 0) d += nd; }
  std::vector<std::uint8_t> seen((std::size_t)nd, 0);
  for (auto d : dd) {
    PLUSS_CHECK(d >= 0 && d < nd, "permute: dim out of range");
    PLUSS_CHECK(!seen[(std::size_t)d], "permute: duplicate dim");
    seen[(std::size_t)d] = 1;
  }
  auto ns = std::vector<std::int64_t>(nd);
  auto nt = std::vector<std::int64_t>(nd);
  for (std::int64_t i=0;i<nd;++i) {
    ns[(std::size_t)i] = sizes()[(std::size_t)dd[(std::size_t)i]];
    nt[(std::size_t)i] = strides()[(std::size_t)dd[(std::size_t)i]];
  }
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::move(ns), std::move(nt),
      scalar_type(),
      platform());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  std::vector<std::int64_t> inv(static_cast<std::size_t>(nd), 0);
  for (std::int64_t i = 0; i < nd; ++i) {
    inv[static_cast<std::size_t>(dd[static_cast<std::size_t>(i)])] = i;
  }
  maybe_link_unary_op_(
      out,
      *this,
      [inv](const Series& out_sens) {
        return out_sens.permute(inv);
      });
  maybe_link_alias_lineage_(out, *this, "permute");
  return out;
}

Series Series::unsqueeze(std::int64_t dim_) const {
  if (!defined()) PLUSS_FAIL("unsqueeze: undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = dim();
  if (dim_ < 0) dim_ += (nd + 1);
  PLUSS_CHECK(dim_ >= 0 && dim_ <= nd, "unsqueeze: dim out of range");
  auto ns = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  auto nt = std::vector<std::int64_t>(strides().begin(), strides().end());
  ns.insert(ns.begin() + dim_, 1);
  std::int64_t stride = 1;
  if (nd == 0) stride = 1;
  else if (dim_ == nd) stride = nt.back();
  else stride = nt[(std::size_t)dim_] * ns[(std::size_t)dim_+1];
  nt.insert(nt.begin() + dim_, stride);
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::move(ns), std::move(nt),
      scalar_type(),
      platform());
  const auto orig_shape = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  maybe_link_unary_op_(
      out,
      *this,
      [shape = std::move(orig_shape)](const Series& out_sens) {
        return out_sens.reshape(shape);
      });
  maybe_link_alias_lineage_(out, *this, "unsqueeze");
  return out;
}

Series Series::squeeze() const {
  if (!defined()) PLUSS_FAIL("squeeze: undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = dim();
  std::vector<std::int64_t> ns;
  std::vector<std::int64_t> nt;
  ns.reserve((std::size_t)nd);
  nt.reserve((std::size_t)nd);
  for (std::int64_t i=0;i<nd;++i) {
    if (sizes()[(std::size_t)i] == 1) continue;
    ns.push_back(sizes()[(std::size_t)i]);
    nt.push_back(strides()[(std::size_t)i]);
  }
  if (ns.empty()) { ns.push_back(1); nt.push_back(1); }
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::move(ns), std::move(nt),
      scalar_type(),
      platform());
  const auto orig_shape = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  maybe_link_unary_op_(
      out,
      *this,
      [shape = std::move(orig_shape)](const Series& out_sens) {
        return out_sens.reshape(shape);
      });
  maybe_link_alias_lineage_(out, *this, "squeeze");
  return out;
}

Series Series::squeeze(std::int64_t dim_) const {
  if (!defined()) PLUSS_FAIL("squeeze(dim): undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = dim();
  if (dim_ < 0) dim_ += nd;
  PLUSS_CHECK(dim_ >= 0 && dim_ < nd, "squeeze(dim): dim out of range");
  if (sizes()[(std::size_t)dim_] != 1) return *this;
  auto ns = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  auto nt = std::vector<std::int64_t>(strides().begin(), strides().end());
  ns.erase(ns.begin() + dim_);
  nt.erase(nt.begin() + dim_);
  if (ns.empty()) { ns.push_back(1); nt.push_back(1); }
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::move(ns), std::move(nt),
      scalar_type(),
      platform());
  const auto orig_shape = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
  maybe_link_unary_op_(
      out,
      *this,
      [shape = std::move(orig_shape)](const Series& out_sens) {
        return out_sens.reshape(shape);
      });
  maybe_link_alias_lineage_(out, *this, "squeeze");
  return out;
}

Series Series::flatten(std::int64_t start_dim, std::int64_t end_dim) const {
  if (!defined()) PLUSS_FAIL("flatten: undefined");
  const auto nd = dim();
  if (start_dim < 0) start_dim += nd;
  if (end_dim < 0) end_dim += nd;
  PLUSS_CHECK(start_dim >= 0 && start_dim < nd, "flatten: start_dim out of range");
  PLUSS_CHECK(end_dim >= 0 && end_dim < nd, "flatten: end_dim out of range");
  PLUSS_CHECK(start_dim <= end_dim, "flatten: start_dim must be <= end_dim");

  std::vector<std::int64_t> new_sizes;
  new_sizes.reserve((std::size_t)nd - (std::size_t)(end_dim-start_dim));
  for (std::int64_t i=0;i<start_dim;++i) new_sizes.push_back(sizes()[(std::size_t)i]);
  std::int64_t flat = 1;
  for (std::int64_t i=start_dim;i<=end_dim;++i) flat *= sizes()[(std::size_t)i];
  new_sizes.push_back(flat);
  for (std::int64_t i=end_dim+1;i<nd;++i) new_sizes.push_back(sizes()[(std::size_t)i]);

  return reshape(new_sizes);
}

Series Series::norm(double p) const {
  if (!defined()) PLUSS_FAIL("norm: undefined");
  // complex norms use abs-based definition

  if (p == 2.0) {
    Series mag = this->abs();
    auto y = ops_internal::sum(mag.mul(mag));
    return y.sqrt();
  }
  if (p == 1.0) {
    return ops_internal::sum(this->abs());
  }
  if (std::isinf(p)) {
    return ops_internal::amax(this->abs());
  }
  // general p-norm: (sum |x|^p)^(1/p)
  auto y = ops_internal::sum(this->abs().pow(p));
  return y.pow(1.0/p);
}

Series Series::norm(double p, std::int64_t dim_, bool keepdim) const {
  if (!defined()) PLUSS_FAIL("norm(dim): undefined");
  // complex norms use abs-based definition

  if (p == 2.0) {
    Series mag = this->abs();
    auto y = mag.mul(mag).sum(dim_, keepdim);
    return y.sqrt();
  }
  if (p == 1.0) {
    return this->abs().sum(dim_, keepdim);
  }
  if (std::isinf(p)) {
    return this->abs().amax(dim_, keepdim);
  }
  auto y = this->abs().pow(p).sum(dim_, keepdim);
  return y.pow(1.0 / p);
}

static void check_float_for_var(const Series& a, const char* name) {
  const auto dt = a.scalar_type();
  const bool ok = (dt == ScalarType::Float64 || dt == ScalarType::Float32 || dt == ScalarType::Float16 || dt == ScalarType::BFloat16);
  PLUSS_CHECK(ok, std::string(name) + ": expects floating dtype");
}

Series Series::var(bool unbiased) const {
  check_float_for_var(*this, "var");
  auto aa = contiguous();
  const auto n = aa.numel();
  PLUSS_CHECK(n > 0, "var: empty");

  // Backend-agnostic formulation that keeps computation on the active platform.
  ScalarType acc_dt = aa.scalar_type();
  if (acc_dt == ScalarType::Float16 || acc_dt == ScalarType::BFloat16) {
    acc_dt = ScalarType::Float32;
  }
  auto x = aa.to(acc_dt);
  auto mu = x.mean();
  auto centered = x.sub(mu);
  auto v = centered.mul(centered).mean();

  if (unbiased) {
    PLUSS_CHECK(n > 1, "var(unbiased): n must be > 1");
    const double corr = static_cast<double>(n) / static_cast<double>(n - 1);
    v = v.mul(corr);
  }
  return v.to(scalar_type());
}

Series Series::var(std::int64_t dim_, bool unbiased, bool keepdim) const {
  check_float_for_var(*this, "var(dim)");
  auto m = mean(dim_, true);
  auto diff = this->sub(m);
  auto v = diff.mul(diff).mean(dim_, keepdim);
  if (unbiased) {
    const double n = (double)sizes()[(std::size_t)((dim_<0)?(dim_+dim()):dim_)];
    if (n > 1.0) {
      const std::array<std::int64_t, 1> one{1};
      auto corr = Series::full(std::span<const std::int64_t>(one.data(), one.size()), n/(n-1.0), v.scalar_type())
                      .to(v.platform());
      v = v.mul(corr);
    }
  }
  return v;
}

Series Series::std(bool unbiased) const {
  return var(unbiased).sqrt();
}

Series Series::std(std::int64_t dim_, bool unbiased, bool keepdim) const {
  return var(dim_, unbiased, keepdim).sqrt();
}
Series Series::clone() const {
  if (!defined()) PLUSS_FAIL("clone: undefined");

  Series out = Series::empty(sizes(), scalar_type(), platform());
  SeriesIterator it = SeriesIterator::unary(*this, out);
  it.copy_out_from_in(*this, out);
  return out;
}

Series Series::contiguous() const {
  if (!defined()) PLUSS_FAIL("contiguous: undefined");
  if (is_contiguous()) return *this;

  Series out = Series::empty(sizes(), scalar_type(), platform());
  SeriesIterator it = SeriesIterator::unary(*this, out);
  it.copy_out_from_in(*this, out);
  // Preserve reverse-mode connectivity through contiguous staging copies.
  maybe_link_unary_op_(out, *this, "contiguous", [](const Series& g) { return g; });
  return out;
}

Series Series::platform(Platform target) const {
  if (!defined()) PLUSS_FAIL("platform: undefined");
  const auto src_platform = platform();
  if (src_platform == target) {
    return *this;
  }

  switch (target) {
    case Platform::CPU:
      break;
    case Platform::CUDA:
      if (!native::cuda::runtime_available()) {
        PLUSS_FAIL(
            std::string("Series::platform(CUDA): CUDA runtime unavailable: ") +
            native::cuda::runtime_error_cstr());
      }
      break;
    case Platform::MPS:
      if (!native::mps::runtime_available()) {
        PLUSS_FAIL(
            std::string("Series::platform(MPS): MPS runtime unavailable: ") +
            native::mps::runtime_error_cstr());
      }
      PLUSS_CHECK(
          scalar_type() != ScalarType::Float64,
          "Series::platform(MPS): float64 is unsupported on MPS; cast to float32/float16/bfloat16 first");
      break;
    default:
      PLUSS_FAIL("Series::platform: unsupported platform");
  }

  auto src = contiguous();
  Series out = Series::empty(src.sizes(), src.scalar_type(), target);
  const auto nbytes =
      static_cast<std::size_t>(src.numel()) * static_cast<std::size_t>(src.itemsize());
  if (nbytes > 0) {
    std::memcpy(out.raw_mutable_data(), src.raw_data(), nbytes);
  }
  maybe_link_transport_op_(
      out,
      *this,
      std::string("platform(") + std::string(platform_name_(target)) + ")");
  return out;
}

Series Series::to(Platform target) const {
  return platform(target);
}

Series Series::to(Platform target, ScalarType dst) const {
  if (target == Platform::MPS) {
    PLUSS_CHECK(dst != ScalarType::Float64,
                "Series::to(MPS, float64): float64 is unsupported on MPS");
  }
  return to(target).to(dst);
}

#ifdef PLUSS_ENABLE_TORCH
::torch::Tensor Series::tensor() const {
  return pluss::codem::to_torch(*this);
}
#endif

Series Series::to(ScalarType dst) const {
  if (!defined()) PLUSS_FAIL("to: undefined");
  if (dst == scalar_type()) return *this;
  if (platform() == Platform::MPS) {
    PLUSS_CHECK(dst != ScalarType::Float64,
                "Series::to(float64): float64 is unsupported on MPS");
  }

  auto src = contiguous();
  Series out = Series::empty(src.sizes(), dst, platform());
  const auto n = src.numel();

  if (platform() == Platform::MPS) {
    const bool supported = native::mps::supports_cast(src.scalar_type(), dst);
    if (supported) {
      native::mps::cast(src, out);
      maybe_link_transport_op_(
          out,
          *this,
          std::string("to(") + std::to_string(static_cast<int>(dst)) + ")");
      return out;
    }
    require_mps_support_or_policy_(false, "Series::to(dtype)");
    Series cpu_cast = to(Platform::CPU).to(dst);
    Series bridged = cpu_cast.to(Platform::MPS);
    maybe_link_transport_op_(
        bridged,
        *this,
        std::string("to(") + std::to_string(static_cast<int>(dst)) + ")");
    return bridged;
  }

  auto store_bool = [&](std::int64_t i, bool v) {
    reinterpret_cast<std::uint8_t*>(out.raw_data())[i] = v ? 1 : 0;
  };

  auto load_as_f64 = [&](ScalarType st, std::int64_t i) -> double {
    switch (st) {
      case ScalarType::Bool: return reinterpret_cast<const std::uint8_t*>(src.raw_data())[i] ? 1.0 : 0.0;
      case ScalarType::UInt8: return reinterpret_cast<const std::uint8_t*>(src.raw_data())[i];
      case ScalarType::UInt16: return reinterpret_cast<const std::uint16_t*>(src.raw_data())[i];
      case ScalarType::UInt32: return reinterpret_cast<const std::uint32_t*>(src.raw_data())[i];
      case ScalarType::UInt64: return static_cast<double>(reinterpret_cast<const std::uint64_t*>(src.raw_data())[i]);
      case ScalarType::Int8: return reinterpret_cast<const std::int8_t*>(src.raw_data())[i];
      case ScalarType::Int16: return reinterpret_cast<const std::int16_t*>(src.raw_data())[i];
      case ScalarType::Int32: return reinterpret_cast<const std::int32_t*>(src.raw_data())[i];
      case ScalarType::Int64: return static_cast<double>(reinterpret_cast<const std::int64_t*>(src.raw_data())[i]);
      case ScalarType::Float16: return float16_to_float(reinterpret_cast<const Float16*>(src.raw_data())[i]);
      case ScalarType::BFloat16: return bfloat16_to_float(reinterpret_cast<const BFloat16*>(src.raw_data())[i]);
      case ScalarType::Float8E4M3FN: return float8e4m3fn_to_float(reinterpret_cast<const Float8E4M3FN*>(src.raw_data())[i]);
      case ScalarType::Float8E5M2: return float8e5m2_to_float(reinterpret_cast<const Float8E5M2*>(src.raw_data())[i]);
      case ScalarType::Float32: return reinterpret_cast<const float*>(src.raw_data())[i];
      case ScalarType::Float64: return reinterpret_cast<const double*>(src.raw_data())[i];
      case ScalarType::Complex64:
        return static_cast<double>(reinterpret_cast<const Complex64*>(src.raw_data())[i].real());
      case ScalarType::Complex128:
        return reinterpret_cast<const Complex128*>(src.raw_data())[i].real();
      default: PLUSS_FAIL("to: unsupported source dtype"); return 0.0;
    }
  };

  auto store_from_f64 = [&](ScalarType st, std::int64_t i, double v) {
    switch (st) {
      case ScalarType::Bool: store_bool(i, v != 0.0); return;
      case ScalarType::UInt8: reinterpret_cast<std::uint8_t*>(out.raw_data())[i] = static_cast<std::uint8_t>(v); return;
      case ScalarType::UInt16: reinterpret_cast<std::uint16_t*>(out.raw_data())[i] = static_cast<std::uint16_t>(v); return;
      case ScalarType::UInt32: reinterpret_cast<std::uint32_t*>(out.raw_data())[i] = static_cast<std::uint32_t>(v); return;
      case ScalarType::UInt64: reinterpret_cast<std::uint64_t*>(out.raw_data())[i] = static_cast<std::uint64_t>(v); return;
      case ScalarType::Int8: reinterpret_cast<std::int8_t*>(out.raw_data())[i] = static_cast<std::int8_t>(v); return;
      case ScalarType::Int16: reinterpret_cast<std::int16_t*>(out.raw_data())[i] = static_cast<std::int16_t>(v); return;
      case ScalarType::Int32: reinterpret_cast<std::int32_t*>(out.raw_data())[i] = static_cast<std::int32_t>(v); return;
      case ScalarType::Int64: reinterpret_cast<std::int64_t*>(out.raw_data())[i] = static_cast<std::int64_t>(v); return;
      case ScalarType::Float16: reinterpret_cast<Float16*>(out.raw_data())[i] = float16_from_float(static_cast<float>(v)); return;
      case ScalarType::BFloat16: reinterpret_cast<BFloat16*>(out.raw_data())[i] = bfloat16_from_float(static_cast<float>(v)); return;
      case ScalarType::Float8E4M3FN: reinterpret_cast<Float8E4M3FN*>(out.raw_data())[i] = float8e4m3fn_from_float(static_cast<float>(v)); return;
      case ScalarType::Float8E5M2: reinterpret_cast<Float8E5M2*>(out.raw_data())[i] = float8e5m2_from_float(static_cast<float>(v)); return;
      case ScalarType::Float32: reinterpret_cast<float*>(out.raw_data())[i] = static_cast<float>(v); return;
      case ScalarType::Float64: reinterpret_cast<double*>(out.raw_data())[i] = v; return;
      case ScalarType::Complex64:
        reinterpret_cast<Complex64*>(out.raw_data())[i] = Complex64(static_cast<float>(v), 0.0f);
        return;
      case ScalarType::Complex128:
        reinterpret_cast<Complex128*>(out.raw_data())[i] = Complex128(v, 0.0);
        return;
      default: PLUSS_FAIL("to: unsupported destination dtype"); return;
    }
  };

  for (std::int64_t i=0; i<n; ++i) {
    const double v = load_as_f64(src.scalar_type(), i);
    store_from_f64(dst, i, v);
  }
  maybe_link_transport_op_(
      out,
      *this,
      std::string("to(") + std::to_string(static_cast<int>(dst)) + ")");
  return out;
}

Series Series::select(std::int64_t dim, std::int64_t index) const {
  if (!defined()) PLUSS_FAIL("select: undefined");
  sync_mps_alias_source_if_needed_(*this);
  const auto nd = this->dim();
  PLUSS_CHECK(nd > 0, "select: cannot be applied to a 0-dim Series");
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "select: dim out of range");  auto sizes_v = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  auto strides_v = std::vector<std::int64_t>(strides().begin(), strides().end());
  const auto size_dim = sizes_v[static_cast<std::size_t>(dim)];
  if (index < 0) index += size_dim;
  if (index < 0 || index >= size_dim) PLUSS_FAIL("select: index out of range");
  // Remove dimension, adjust offset
  const auto offset_add = index * strides_v[static_cast<std::size_t>(dim)];
  sizes_v.erase(sizes_v.begin() + dim);
  strides_v.erase(strides_v.begin() + dim);

  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(),
      impl_->storage_offset_elems() + offset_add,
      std::move(sizes_v),
      std::move(strides_v),
      scalar_type(),
      platform());
  Series out = ops_internal::SeriesFactoryAccess::wrap(std::move(impl));

  maybe_link_unary_op_(
      out,
      *this,
      "select",
      [input = *this, selected = out, dim, index](const Series& out_sens) {
        Series sens = out_sens;
        if (!same_shape_(sens, selected)) {
          sens = reduce_broadcasted_sensitivity_to_like_(sens, selected, "select");
        }
        if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
        if (sens.platform() != input.platform()) sens = sens.to(input.platform());
        Series grad = Series::zeros(input.sizes(), input.scalar_type()).to(input.platform());
        Series sens_expanded = sens.unsqueeze(dim);
        Series idx = Series::full(sens_expanded.sizes(), static_cast<double>(index), ScalarType::Int64).to(input.platform());
        return ops_internal::scatter(grad, dim, idx, sens_expanded);
      });
  maybe_link_alias_lineage_(out, *this, "select");
  return out;
}

Series Series::slice(
    std::int64_t dim,
    std::int64_t start,
    std::int64_t end,
    std::int64_t step) const {
  PLUSS_CHECK(defined(), "Series::slice: undefined Series");
  const std::int64_t ndim = this->dim();
  std::int64_t d = dim;
  if (d < 0) d += ndim;
  PLUSS_CHECK(d >= 0 && d < ndim, "Series::slice: dim out of range");
  PLUSS_CHECK(step != 0, "Series::slice: step must be non-zero");

  const std::int64_t n = size(d);
  auto normalize_bound = [n](std::int64_t v, bool is_end) -> std::int64_t {
    if (v == std::numeric_limits<std::int64_t>::max()) return is_end ? n : 0;
    if (v < 0) v += n;
    if (v < 0) return 0;
    if (v > n) return n;
    return v;
  };

  const std::int64_t s = normalize_bound(start, false);
  const std::int64_t e = normalize_bound(end, true);

  PLUSS_CHECK(step > 0,
              "Series::slice: negative-step slices are not supported by this cheap view helper");

  const std::int64_t len = (e <= s) ? 0 : (1 + (e - 1 - s) / step);

  const auto size_view = sizes();
  const auto stride_view = strides();
  std::vector<std::int64_t> out_sizes(size_view.begin(), size_view.end());
  std::vector<std::int64_t> out_strides(stride_view.begin(), stride_view.end());

  const std::size_t ud = static_cast<std::size_t>(d);
  const std::int64_t base_stride = out_strides[ud];
  out_sizes[ud] = len;
  out_strides[ud] = base_stride * step;

  const std::int64_t base_off_elems =
      storage_offset() / std::max<std::int64_t>(1, itemsize());
  const std::int64_t view_off_elems = base_off_elems + s * base_stride;

  Series out = Series::from_storage(
      storage(),
      view_off_elems,
      out_sizes,
      out_strides,
      scalar_type());

  maybe_link_unary_op_(
      out,
      *this,
      "slice",
      [input = *this,
       d,
       s,
       step,
       len](const Series& out_sens) {
        Series sens = out_sens;
        if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
        if (sens.platform() != input.platform()) sens = sens.to(input.platform());

        Series grad = Series::zeros(input.sizes(), input.scalar_type()).to(input.platform());
        if (sens.numel() == 0) return grad;

        std::vector<std::int64_t> ids;
        ids.reserve(static_cast<std::size_t>(len));
        for (std::int64_t i = 0; i < len; ++i) {
          ids.push_back(s + i * step);
        }
        Series idx = i64_index_vector_on_(ids, input.platform());
        std::vector<std::int64_t> idx_shape(static_cast<std::size_t>(input.dim()), 1);
        idx_shape[static_cast<std::size_t>(d)] = len;
        idx = idx.reshape(idx_shape).expand(sens.sizes(), true);
        return ops_internal::scatter(grad, d, idx, sens);
      });
  maybe_link_alias_lineage_(out, *this, "slice");
  return out;
}

static void ensure_writable_(Series& self) {
  if (!self.defined()) PLUSS_FAIL("inplace: undefined");
}

static void copy_into_existing_(Series& dst, const Series& src) {
  PLUSS_CHECK(dst.defined() && src.defined(), "inplace copy: undefined");
  require_same_platform(dst, src, "inplace copy");
  PLUSS_CHECK(dst.scalar_type() == src.scalar_type(), "inplace copy: dtype mismatch");
  PLUSS_CHECK(dst.sizes().size() == src.sizes().size(), "inplace copy: rank mismatch");
  for (std::size_t i = 0; i < dst.sizes().size(); ++i) {
    PLUSS_CHECK(dst.sizes()[i] == src.sizes()[i], "inplace copy: shape mismatch");
  }
  if (dst.is_contiguous() && src.is_contiguous() && dst.numel() == src.numel() && dst.itemsize() == src.itemsize()) {
    const auto nbytes = static_cast<std::size_t>(dst.numel()) * static_cast<std::size_t>(dst.itemsize());
    if (nbytes > 0) {
      std::memcpy(dst.raw_mutable_data(), src.raw_data(), nbytes);
    }
    return;
  }
  SeriesIterator it = SeriesIterator::unary(src, dst);
  it.copy_out_from_in(src, dst);
}

static Series expand_to_self_(const Series& rhs, const Series& self) {
  // If shapes match, ok
  if (std::vector<std::int64_t>(rhs.sizes().begin(), rhs.sizes().end()) ==
      std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end())) {
    return rhs;
  }
  // Allow broadcast via expand if rhs can expand to self sizes
  return rhs.expand(self.sizes(), /*implicit=*/true);
}

static void guard_direct_inplace_value_op_(std::string_view op, const Series& self) {
  if (!series_tape::recording_enabled()) return;
  if (!series_tape::tracks_slope(self)) return;
  PLUSS_FAIL(
      std::string(op) +
      ": in-place mutation on autograd-tracked Series is disallowed; mutate a detached/separate Series instead");
}

static Series& finish_direct_inplace_value_op_(Series& self) {
  if (self.defined()) {
    self.storage().bump_version();
  }
  return self;
}

Series& Series::add_(const Series& rhs, double alpha) {
  guard_direct_inplace_value_op_("add_", *this);
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "add_");
  Series b = expand_to_self_(rhs, *this).to(scalar_type());
  if (alpha != 1.0) {
    b = b.mul(alpha).to(scalar_type());
  }
  Series out = ops_internal::add(*this, b);
  copy_into_existing_(*this, out);
  return finish_direct_inplace_value_op_(*this);
}

Series& Series::add_(const Series& other, double alpha, const Series& out) {
  // The generated signature provides an 'out' argument that is const; treat it as
  // an aliasing hint and validate if possible.
  if (out.defined()) {
    PLUSS_CHECK(out.unsafe_impl() == this->unsafe_impl(), "add_: 'out' must alias self");
  }
  return add_(other, alpha);
}

Series& Series::add_(double other, double alpha) {
  // Promote scalar to Series and delegate.
  const std::array<std::int64_t, 1> one{1};
  auto s = Series::full(std::span<const std::int64_t>(one.data(), one.size()), other, scalar_type()).to(platform());
  auto fn = static_cast<Series&(Series::*)(const Series&, double)>(&Series::add_);
  return (this->*fn)(s, alpha);
}

Series& Series::sub_(const Series& rhs, double alpha) {
  guard_direct_inplace_value_op_("sub_", *this);
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "sub_");
  Series b = expand_to_self_(rhs, *this).to(scalar_type());
  if (alpha != 1.0) {
    b = b.mul(alpha).to(scalar_type());
  }
  Series out = ops_internal::sub(*this, b);
  copy_into_existing_(*this, out);
  return finish_direct_inplace_value_op_(*this);
}

Series& Series::sub_(const Series& other, double alpha, const Series& out) {
  if (out.defined()) {
    PLUSS_CHECK(out.unsafe_impl() == this->unsafe_impl(), "sub_: 'out' must alias self");
  }
  return sub_(other, alpha);
}

Series& Series::sub_(double other, double alpha) {
  const std::array<std::int64_t, 1> one{1};
  auto s = Series::full(std::span<const std::int64_t>(one.data(), one.size()), other, scalar_type()).to(platform());
  auto fn = static_cast<Series&(Series::*)(const Series&, double)>(&Series::sub_);
  return (this->*fn)(s, alpha);
}

Series& Series::mul_(const Series& rhs) {
  guard_direct_inplace_value_op_("mul_", *this);
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "mul_");
  auto b = expand_to_self_(rhs, *this).to(scalar_type());
  Series out = ops_internal::mul(*this, b);
  copy_into_existing_(*this, out);
  return finish_direct_inplace_value_op_(*this);
}

Series& Series::div_(const Series& rhs) {
  guard_direct_inplace_value_op_("div_", *this);
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "div_");
  auto b = expand_to_self_(rhs, *this).to(scalar_type());
  Series out = ops_internal::div(*this, b);
  copy_into_existing_(*this, out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::arange(double start, double end, double step, ScalarType dtype) {
  if (step == 0.0) PLUSS_FAIL("arange: step cannot be 0");
  const double nreal = (end - start) / step;
  std::int64_t n = (std::int64_t)std::ceil(nreal);
  if (n < 0) n = 0;
  Series out = Series::empty({n}, dtype);
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

Series Series::linspace(double start, double end, std::int64_t steps, ScalarType dtype) {
  if (steps < 0) PLUSS_FAIL("linspace: steps must be >= 0");
  Series out = Series::empty({steps}, dtype);
  if (steps == 0) return out;
  if (steps == 1) {
    return Series::full({1}, start, dtype);
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

Series Series::eye(std::int64_t n, std::int64_t m, ScalarType dtype) {
  if (m < 0) {
    m = n;
  }
  if (n < 0 || m < 0) PLUSS_FAIL("eye: sizes must be >= 0");
  Series out = Series::zeros({n,m}, dtype);
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

Series Series::rand(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    PLUSS_FAIL("rand: dtype must be float");
  Series out = Series::empty(sizes, dtype);
  auto state = pluss::aux::seed_or_next(seed);
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

Series Series::randn(pluss::msharp::RangeView::Int sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    PLUSS_FAIL("randn: dtype must be float");
  Series out = Series::empty(sizes, dtype);
  auto state = pluss::aux::seed_or_next(seed);
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

Series Series::from_storage(const Storage& storage, std::int64_t storage_offset,
                            const std::vector<std::int64_t>& sizes,
                            const std::vector<std::int64_t>& strides,
                            ScalarType dtype) {
  if (!storage.defined()) PLUSS_FAIL("from_storage: undefined storage");
  // Interop / external views must be non-overlapping & dense (debug-enforced).
  check_view_invariants(sizes, strides);
  return ops_internal::SeriesFactoryAccess::wrap(
      pluss::msharp::make_intrusive<SeriesImpl>(storage, storage_offset, sizes, strides, dtype, Platform::CPU));
}

static Series bitwise_binary_(
    const Series& a,
    const Series& b,
    void(*fn)(const Series&, const Series&, const Series&),
    std::string_view op_name) {
  require_same_platform(a, b, "bitwise op");
  auto cpu_route = [&]() -> Series {
    auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
    Series out = Series::empty(shape, a.scalar_type(), a.platform());
    fn(a.expand(out.sizes()), b.expand(out.sizes()), out);
    return out;
  };
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_bitwise_enabled();
    const bool native_strict = native::cuda::native_bitwise_strict();
    const bool route_supported = native::cuda::supports_bitwise_binary(a, b);
    return cuda_bitwise_route_or_bridge_(
        op_name,
        native_enabled,
        native_strict,
        route_supported,
        cpu_route);
  }
  require_backend_dispatch_supported_(a.platform(), op_name);
  return cpu_route();
}

enum class MpsBitwiseOp : std::uint8_t { And, Or, Xor };

static bool is_integral_or_bool_dtype_(ScalarType dt) noexcept {
  switch (dt) {
    case ScalarType::Bool:
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
    case ScalarType::UInt8:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
    case ScalarType::UInt64:
      return true;
    default:
      return false;
  }
}

static bool mps_take_put_exact_f32_roundtrip_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float32:
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Bool:
    case ScalarType::Int8:
    case ScalarType::UInt8:
    case ScalarType::Int16:
    case ScalarType::UInt16:
      return true;
    default:
      return false;
  }
}

static bool mps_take_fast_path_supported_(const Series& self, const Series& index) noexcept {
  if (!self.defined() || !index.defined()) return false;
  if (self.platform() != Platform::MPS || index.platform() != Platform::MPS) return false;
  if (index.scalar_type() != ScalarType::Int64) return false;
  const bool direct_lane_dtype =
      self.scalar_type() == ScalarType::Float32 || self.scalar_type() == ScalarType::Int32;
  if (direct_lane_dtype && native::mps::supports_gather(self.scalar_type(), ScalarType::Int64)) return true;
  const bool supports_f32 = native::mps::supports_gather(ScalarType::Float32, ScalarType::Int64);
  const bool cast_lane =
      supports_f32 &&
      mps_take_put_exact_f32_roundtrip_dtype_(self.scalar_type()) &&
      native::mps::supports_cast(self.scalar_type(), ScalarType::Float32) &&
      native::mps::supports_cast(ScalarType::Float32, self.scalar_type());
  return cast_lane;
}

static bool mps_put_fast_path_supported_(
    const Series& self,
    const Series& index,
    const Series& source,
    bool accumulate) noexcept {
  if (!self.defined() || !index.defined() || !source.defined()) return false;
  if (self.platform() != Platform::MPS || index.platform() != Platform::MPS || source.platform() != Platform::MPS) {
    return false;
  }
  if (index.scalar_type() != ScalarType::Int64) return false;

  const bool source_shape_ok = source.numel() == 1 || source.numel() == index.numel();
  if (!source_shape_ok) return false;

  const bool direct_set = native::mps::supports_scatter(self.scalar_type(), ScalarType::Int64, self.scalar_type());
  const bool direct_reduce =
      !accumulate || native::mps::supports_scatter_reduce(self.scalar_type(), ScalarType::Int64, self.scalar_type());
  const bool source_direct_cast =
      source.scalar_type() == self.scalar_type() ||
      native::mps::supports_cast(source.scalar_type(), self.scalar_type());
  if (direct_set && direct_reduce && source_direct_cast) {
    return true;
  }

  const bool supports_f32_set =
      native::mps::supports_scatter(ScalarType::Float32, ScalarType::Int64, ScalarType::Float32);
  const bool f32_roundtrip =
      mps_take_put_exact_f32_roundtrip_dtype_(self.scalar_type()) &&
      native::mps::supports_cast(self.scalar_type(), ScalarType::Float32) &&
      native::mps::supports_cast(ScalarType::Float32, self.scalar_type());
  const bool source_f32_cast =
      source.scalar_type() == ScalarType::Float32 ||
      mps_take_put_exact_f32_roundtrip_dtype_(source.scalar_type()) ||
      native::mps::supports_cast(source.scalar_type(), ScalarType::Float32);
  return supports_f32_set && !accumulate && f32_roundtrip && source_f32_cast;
}

static bool has_duplicate_linear_indices_(const Series& index_cpu_i64_contig, std::int64_t total_numel) {
  PLUSS_CHECK(index_cpu_i64_contig.platform() == Platform::CPU, "duplicate-index check: expected CPU index");
  PLUSS_CHECK(index_cpu_i64_contig.scalar_type() == ScalarType::Int64 && index_cpu_i64_contig.dim() == 1,
              "duplicate-index check: expected int64 1D index");
  const auto n = index_cpu_i64_contig.numel();
  if (n <= 1) return false;

  const auto* ip = reinterpret_cast<const std::int64_t*>(index_cpu_i64_contig.raw_data());
  std::vector<std::int64_t> normalized(static_cast<std::size_t>(n), 0);
  for (std::int64_t i = 0; i < n; ++i) {
    std::int64_t k = ip[i];
    if (k < 0) k += total_numel;
    if (k < 0 || k >= total_numel) {
      // Defer exact exception behavior to canonical CPU put implementation.
      return true;
    }
    normalized[static_cast<std::size_t>(i)] = k;
  }
  std::sort(normalized.begin(), normalized.end());
  for (std::size_t i = 1; i < normalized.size(); ++i) {
    if (normalized[i] == normalized[i - 1]) return true;
  }
  return false;
}

static Series bitwise_binary_mps_(const Series& a, const Series& b, MpsBitwiseOp op) {
  require_same_platform(a, b, "bitwise op");
  PLUSS_CHECK(
      is_integral_or_bool_dtype_(a.scalar_type()) && is_integral_or_bool_dtype_(b.scalar_type()),
      "bitwise op: expected integral/bool inputs");
  const auto out_dt = promote_types(a.scalar_type(), b.scalar_type());
  PLUSS_CHECK(is_integral_or_bool_dtype_(out_dt), "bitwise op: unsupported promoted dtype");

  const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  const Series aa = a.expand(shape, true).to(ScalarType::Int64).contiguous();
  const Series bb = b.expand(shape, true).to(ScalarType::Int64).contiguous();
  Series out_i64 = Series::empty(shape, ScalarType::Int64, a.platform());

  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* optr = reinterpret_cast<std::int64_t*>(out_i64.raw_mutable_data());
  const auto n = out_i64.numel();
  for (std::int64_t i = 0; i < n; ++i) {
    switch (op) {
      case MpsBitwiseOp::And: optr[i] = ap[i] & bp[i]; break;
      case MpsBitwiseOp::Or: optr[i] = ap[i] | bp[i]; break;
      case MpsBitwiseOp::Xor: optr[i] = ap[i] ^ bp[i]; break;
    }
  }

  if (out_dt == ScalarType::Int64) return out_i64;
  if (out_dt == ScalarType::Bool) return out_i64.ne(0.0);
  return out_i64.to(out_dt);
}

static Series bitwise_not_mps_(const Series& a) {
  PLUSS_CHECK(is_integral_or_bool_dtype_(a.scalar_type()), "bitwise_not: expected integral/bool input");
  if (a.scalar_type() == ScalarType::Bool) {
    return ops_internal::logical_not(a);
  }
  const Series ai = a.to(ScalarType::Int64).contiguous();
  Series out_i = Series::empty(ai.sizes(), ScalarType::Int64, a.platform());
  const auto* ip = reinterpret_cast<const std::int64_t*>(ai.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out_i.raw_mutable_data());
  const auto n = ai.numel();
  for (std::int64_t i = 0; i < n; ++i) op[i] = ~ip[i];
  return (a.scalar_type() == ScalarType::Int64) ? out_i : out_i.to(a.scalar_type());
}

static Series as_bool(const Series& x) {
  if (x.scalar_type() == ScalarType::Bool) return x;
  return x.to(ScalarType::Bool);
}

Series Series::bitwise_and(const Series& rhs) const {
  if (platform() == Platform::MPS) {
    require_same_platform(*this, rhs, "bitwise_and");
    if (scalar_type() == ScalarType::Bool && rhs.scalar_type() == ScalarType::Bool) {
      return ops_internal::logical_and(*this, rhs);
    }
    return bitwise_binary_mps_(*this, rhs, MpsBitwiseOp::And);
  }
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_and, "bitwise_and");
}
Series Series::bitwise_or(const Series& rhs) const {
  if (platform() == Platform::MPS) {
    require_same_platform(*this, rhs, "bitwise_or");
    if (scalar_type() == ScalarType::Bool && rhs.scalar_type() == ScalarType::Bool) {
      return ops_internal::logical_or(*this, rhs);
    }
    return bitwise_binary_mps_(*this, rhs, MpsBitwiseOp::Or);
  }
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_or, "bitwise_or");
}
Series Series::bitwise_xor(const Series& rhs) const {
  if (platform() == Platform::MPS) {
    require_same_platform(*this, rhs, "bitwise_xor");
    if (scalar_type() == ScalarType::Bool && rhs.scalar_type() == ScalarType::Bool) {
      return ops_internal::logical_xor(*this, rhs);
    }
    return bitwise_binary_mps_(*this, rhs, MpsBitwiseOp::Xor);
  }
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_xor, "bitwise_xor");
}

Series Series::bitwise_not() const {
  if (platform() == Platform::MPS) {
    return bitwise_not_mps_(*this);
  }
  auto cpu_route = [&]() -> Series {
    Series out = Series::empty(sizes(), scalar_type(), platform());
    native::cpu::bitwise_not(*this, out);
    return out;
  };
  if (platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_bitwise_enabled();
    const bool native_strict = native::cuda::native_bitwise_strict();
    const bool route_supported = native::cuda::supports_bitwise_unary(*this);
    return cuda_bitwise_route_or_bridge_(
        "bitwise_not",
        native_enabled,
        native_strict,
        route_supported,
        cpu_route);
  }
  require_backend_dispatch_supported_(platform(), "bitwise_not");
  return cpu_route();
}

Series Series::logical_and(const Series& rhs) const { return as_bool(*this).bitwise_and(as_bool(rhs)); }
Series Series::logical_or(const Series& rhs) const { return as_bool(*this).bitwise_or(as_bool(rhs)); }
Series Series::logical_xor(const Series& rhs) const { return as_bool(*this).bitwise_xor(as_bool(rhs)); }
Series Series::logical_not() const { return as_bool(*this).bitwise_not(); }

Series Series::exp() const { return ops_internal::exp(*this); }
Series Series::log() const { return ops_internal::log(*this); }
Series Series::sqrt() const { return ops_internal::sqrt(*this); }
Series Series::sin() const { return ops_internal::sin(*this); }
Series Series::cos() const { return ops_internal::cos(*this); }
Series Series::tanh() const { return ops_internal::tanh(*this); }

Series& Series::exp_() { guard_direct_inplace_value_op_("exp_", *this); *this = exp(); return finish_direct_inplace_value_op_(*this); }
Series& Series::log_() { guard_direct_inplace_value_op_("log_", *this); *this = log(); return finish_direct_inplace_value_op_(*this); }
Series& Series::sqrt_() { guard_direct_inplace_value_op_("sqrt_", *this); *this = sqrt(); return finish_direct_inplace_value_op_(*this); }
Series& Series::sin_() { guard_direct_inplace_value_op_("sin_", *this); *this = sin(); return finish_direct_inplace_value_op_(*this); }
Series& Series::cos_() { guard_direct_inplace_value_op_("cos_", *this); *this = cos(); return finish_direct_inplace_value_op_(*this); }
Series& Series::tanh_() { guard_direct_inplace_value_op_("tanh_", *this); *this = tanh(); return finish_direct_inplace_value_op_(*this); }

Series Series::unite(pluss::msharp::RangeView::Of<Series> others, std::int64_t dim, bool strict_dtype) const {
  for (const auto& t : others) {
    require_same_platform(*this, t, "unite");
  }

  std::vector<Series> xs;
  xs.reserve(others.size() + 1);
  xs.push_back(*this);
  for (const auto& t : others) xs.push_back(t);

  if (strict_dtype) {
    const ScalarType ref = xs.front().scalar_type();
    for (const auto& x : xs) {
      PLUSS_CHECK(
          x.scalar_type() == ref,
          "unite: dtype mismatch (set strict_dtype=false to enable promotion/coercion)");
    }
  } else {
    ScalarType promoted = xs.front().scalar_type();
    for (std::size_t i = 1; i < xs.size(); ++i) {
      promoted = promote_types(promoted, xs[i].scalar_type());
    }
    for (auto& x : xs) {
      if (x.scalar_type() != promoted) {
        x = x.to(promoted);
      }
    }
  }

  Series out;
  if (platform() == Platform::MPS) {
    const auto nd = this->dim();
    if (dim < 0) dim += nd;
    PLUSS_CHECK(dim >= 0 && dim < nd, "unite: dim out of range");
    out = ops_internal::mps_cat_along_dim_(xs, dim, "unite");
  } else if (platform() == Platform::CUDA) {
    auto cpu_route = [&]() -> Series {
      return native::cpu::cat(xs, dim);
    };
    const bool native_enabled = native::cuda::native_compose_enabled();
    const bool native_strict = native::cuda::native_compose_strict();
    const bool route_supported = native::cuda::supports_unite_compose(*this, others, dim);
    out = cuda_compose_route_or_bridge_(
        "unite",
        native_enabled,
        native_strict,
        route_supported,
        cpu_route);
  } else {
    require_backend_dispatch_supported_(platform(), "unite");
    out = native::cpu::cat(xs, dim);
  }

  if (series_tape::recording_enabled()) {
    std::vector<Series> parents;
    parents.reserve(others.size() + 1);
    parents.push_back(*this);
    parents.insert(parents.end(), others.begin(), others.end());
    bool any_track = false;
    for (const auto& p : parents) {
      if (series_tape::tracks_slope(p)) {
        any_track = true;
        break;
      }
    }
    if (any_track) {
      const auto nd = dim + (dim < 0 ? this->dim() : 0);
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "unite";
      link->parents = parents;
      link->propagate = [parents, nd](const Series& out_sens) {
        std::int64_t offset = 0;
        for (const Series& p : parents) {
          if (!series_tape::tracks_slope(p)) {
            offset += p.sizes()[static_cast<std::size_t>(nd)];
            continue;
          }
          const std::int64_t width = p.sizes()[static_cast<std::size_t>(nd)];
          std::vector<std::int64_t> idx_vec(static_cast<std::size_t>(width));
          for (std::int64_t i = 0; i < width; ++i) idx_vec[static_cast<std::size_t>(i)] = offset + i;
          const Series idx = i64_index_vector_on_(
              std::span<const std::int64_t>(idx_vec.data(), idx_vec.size()),
              out_sens.platform());
          Series g = out_sens.index_select(nd, idx);
          if (g.scalar_type() != p.scalar_type()) g = g.to(p.scalar_type());
          series_tape::accumulate_sensitivity(p, g);
          offset += width;
        }
      };
      series_tape::bind_source_link(out, std::move(link));
    }
  }
  return out;
}

Series Series::pile(pluss::msharp::RangeView::Of<Series> others, std::int64_t dim) const {
  for (const auto& t : others) {
    require_same_platform(*this, t, "pile");
  }
  Series out;
  if (platform() == Platform::MPS) {
    const auto nd = this->dim() + 1;
    if (dim < 0) dim += nd;
    PLUSS_CHECK(dim >= 0 && dim < nd, "pile: dim out of range");
    std::vector<Series> xs;
    xs.reserve(others.size() + 1);
    xs.push_back(this->unsqueeze(dim).contiguous());
    for (const auto& t : others) xs.push_back(t.unsqueeze(dim).contiguous());
    out = ops_internal::mps_cat_along_dim_(xs, dim, "pile");
  } else if (platform() == Platform::CUDA) {
    auto cpu_route = [&]() -> Series {
      std::vector<Series> xs;
      xs.reserve(others.size() + 1);
      xs.push_back(*this);
      for (const auto& t : others) xs.push_back(t);
      return native::cpu::stack(xs, dim);
    };
    const bool native_enabled = native::cuda::native_compose_enabled();
    const bool native_strict = native::cuda::native_compose_strict();
    const bool route_supported = native::cuda::supports_pile_compose(*this, others, dim);
    out = cuda_compose_route_or_bridge_(
        "pile",
        native_enabled,
        native_strict,
        route_supported,
        cpu_route);
  } else {
    require_backend_dispatch_supported_(platform(), "pile");
    std::vector<Series> xs;
    xs.reserve(others.size() + 1);
    xs.push_back(*this);
    for (const auto& t : others) xs.push_back(t);
    out = native::cpu::stack(xs, dim);
  }

  if (series_tape::recording_enabled()) {
    std::vector<Series> parents;
    parents.reserve(others.size() + 1);
    parents.push_back(*this);
    parents.insert(parents.end(), others.begin(), others.end());
    bool any_track = false;
    for (const auto& p : parents) {
      if (series_tape::tracks_slope(p)) {
        any_track = true;
        break;
      }
    }
    if (any_track) {
      std::int64_t nd = this->dim() + 1;
      std::int64_t dim_norm = dim;
      if (dim_norm < 0) dim_norm += nd;
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "pile";
      link->parents = parents;
      link->propagate = [parents, dim_norm](const Series& out_sens) {
        for (std::int64_t i = 0; i < static_cast<std::int64_t>(parents.size()); ++i) {
          const Series& p = parents[static_cast<std::size_t>(i)];
          if (!series_tape::tracks_slope(p)) continue;
          Series g = out_sens.select(dim_norm, i);
          if (g.scalar_type() != p.scalar_type()) g = g.to(p.scalar_type());
          series_tape::accumulate_sensitivity(p, g);
        }
      };
      series_tape::bind_source_link(out, std::move(link));
    }
  }
  return out;
}

Series Series::gather(std::int64_t dim, const Series& index) const {
  return ops_internal::gather(*this, dim, index);
}
Series Series::factorial(std::int64_t n) const {
  (void)n;
  PLUSS_FAIL("factorial: not implemented");
}
Series Series::scatter(std::int64_t dim, const Series& index, const Series& src) const {
  return ops_internal::scatter(*this, dim, index, src);
}

Series& Series::scatter_(std::int64_t dim, const Series& index, const Series& src) {
  guard_direct_inplace_value_op_("scatter_", *this);
  ops_internal::scatter_(*this, dim, index, src);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::scatter_add(std::int64_t dim, const Series& index, const Series& src) const {
  return ops_internal::scatter_add(*this, dim, index, src);
}

Series& Series::scatter_add_(std::int64_t dim, const Series& index, const Series& src) {
  guard_direct_inplace_value_op_("scatter_add_", *this);
  ops_internal::scatter_add_(*this, dim, index, src);
  return finish_direct_inplace_value_op_(*this);
}

static Series scalar_to_series_(double v, ScalarType dt, Platform platform) {
  // For now: create 1-element Series of dtype matching `dt` (or float64 fallback).
  ScalarType use = dt;
  if (use == ScalarType::Bool) use = ScalarType::Float32;
  Series s = Series::full({1}, v, use).to(platform);
  return s;
}

Series Series::add(double rhs) const { return this->add(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::sub(double rhs) const { return this->sub(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::mul(double rhs) const { return this->mul(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::div(double rhs) const { return this->div(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::pow(double rhs) const { return ops_internal::pow(*this, rhs); }
Series Series::minimum(double rhs) const { return this->minimum(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::maximum(double rhs) const { return this->maximum(scalar_to_series_(rhs, scalar_type(), platform())); }

Series Series::eq(double rhs) const { return this->eq(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::ne(double rhs) const { return this->ne(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::lt(double rhs) const { return this->lt(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::le(double rhs) const { return this->le(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::gt(double rhs) const { return this->gt(scalar_to_series_(rhs, scalar_type(), platform())); }
Series Series::ge(double rhs) const { return this->ge(scalar_to_series_(rhs, scalar_type(), platform())); }

Series& Series::add_(double rhs) { return this->add_(scalar_to_series_(rhs, scalar_type(), platform())); }
Series& Series::sub_(double rhs) { return this->sub_(scalar_to_series_(rhs, scalar_type(), platform())); }
Series& Series::mul_(double rhs) {
  auto fn = static_cast<Series&(Series::*)(const Series&)>(&Series::mul_);
  Series s = scalar_to_series_(rhs, scalar_type(), platform());
  return (this->*fn)(s);
}
Series& Series::div_(double rhs) {
  auto fn = static_cast<Series&(Series::*)(const Series&)>(&Series::div_);
  Series s = scalar_to_series_(rhs, scalar_type(), platform());
  return (this->*fn)(s);
}
Series& Series::pow_(double rhs) {
  guard_direct_inplace_value_op_("pow_", *this);
  Series out = this->pow(rhs);
  this->copy_(out);
  return finish_direct_inplace_value_op_(*this);
}
Series& Series::minimum_(double rhs) {
  guard_direct_inplace_value_op_("minimum_", *this);
  Series out = this->minimum(rhs);
  this->copy_(out);
  return finish_direct_inplace_value_op_(*this);
}
Series& Series::maximum_(double rhs) {
  guard_direct_inplace_value_op_("maximum_", *this);
  Series out = this->maximum(rhs);
  this->copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

static bool is_integer_index_dtype_(ScalarType st) {
  switch (st) {
    case ScalarType::UInt8:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
    case ScalarType::UInt64:
    case ScalarType::Int8:
    case ScalarType::Int16:
    case ScalarType::Int32:
    case ScalarType::Int64:
      return true;
    default:
      return false;
  }
}

static bool supports_tracked_index_backward_(
    const Series& input,
    const std::vector<Series>& indices) {
  if (input.dim() < 1) return false;
  if (indices.size() != 1) return false;
  if (!indices.front().defined()) return false;
  return is_integer_index_dtype_(indices.front().scalar_type());
}

static void bind_cpu_index_backward_(
    const Series& out,
    const Series& input,
    const std::vector<Series>& indices) {
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "index";
  link->parents = {input};
  link->propagate = [input, indices](const Series& out_sens) {
    Series sens = out_sens;
    if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
    if (sens.platform() != Platform::CPU) sens = sens.to(Platform::CPU, input.scalar_type());
    sens = sens.contiguous();

    Series base = Series::zeros(input.sizes(), input.scalar_type());
    Series grad = pluss::codem::native::cpu::index_put(base, indices, sens, /*accumulate=*/true);
    series_tape::accumulate_sensitivity(input, grad);
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void bind_single_integer_index_backward_(
    const Series& out,
    const Series& input,
    const Series& index) {
  const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "index";
  link->parents = {input};
  link->propagate = [input, idx](const Series& out_sens) {
    Series sens = out_sens;
    if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
    if (sens.platform() != input.platform()) sens = sens.to(input.platform(), input.scalar_type());
    sens = sens.contiguous();

    PLUSS_CHECK(input.dim() >= 1, "index: tracked backward expects rank >= 1 input");
    const std::int64_t dim0 = input.sizes()[0];
    PLUSS_CHECK(dim0 > 0, "index: tracked backward does not support empty dim0 inputs");
    const std::int64_t trailing = std::max<std::int64_t>(1, input.numel() / dim0);

    const std::array<std::int64_t, 1> flat_index_shape{{idx.numel()}};
    const std::array<std::int64_t, 2> flat_lane_shape{{idx.numel(), trailing}};
    Series idx_flat = idx.reshape(std::span<const std::int64_t>(flat_index_shape.data(), flat_index_shape.size()))
                          .contiguous();
    idx_flat = idx_flat.remainder(static_cast<double>(dim0));
    idx_flat = where(idx_flat.lt(0.0), idx_flat.add(static_cast<double>(dim0)), idx_flat).to(ScalarType::Int64);
    const std::array<std::int64_t, 2> flat_col_shape{{idx.numel(), 1}};
    const Series idx_col = idx_flat.reshape(std::span<const std::int64_t>(flat_col_shape.data(), flat_col_shape.size()));
    const Series idx_expanded =
        idx_col.expand(std::span<const std::int64_t>(flat_lane_shape.data(), flat_lane_shape.size()), true)
            .contiguous();
    const Series sens_flat =
        sens.reshape(std::span<const std::int64_t>(flat_lane_shape.data(), flat_lane_shape.size()));

    Series base_flat = Series::zeros({dim0, trailing}, input.scalar_type()).to(input.platform());
    Series grad_flat = scatter_add(base_flat, /*dim=*/0, idx_expanded, sens_flat);
    series_tape::accumulate_sensitivity(input, grad_flat.reshape(input.sizes()));
  };
  series_tape::bind_source_link(out, std::move(link));
}

Series Series::index(pluss::msharp::RangeView::Of<ExtractSpec> specs) const {
  const std::vector<ExtractSpec> requested(specs.begin(), specs.end());
  const auto normalized = normalize_extract_specs_(requested, dim());

  auto pack_advanced_indices = [](std::span<const ExtractSpec> in) {
    std::vector<Series> packed;
    packed.reserve(in.size());
    for (const auto& spec : in) {
      PLUSS_CHECK(
          spec.kind == ExtractSpec::Kind::AdvancedIndex,
          "Series::index: internal index packer expects advanced-index specs only");
      PLUSS_CHECK(spec.advanced_impl, "Series::index: advanced index Series is undefined");
      Series idx;
      idx.impl() = spec.advanced_impl;
      packed.push_back(std::move(idx));
    }
    return packed;
  };

  auto dispatch_advanced = [&](const std::vector<Series>& packed_indices) -> Series {
    for (const auto& idx : packed_indices) {
      require_same_platform(*this, idx, "index");
    }

    Series out;
    if (platform() == Platform::MPS) {
      PLUSS_CHECK(packed_indices.size() == 1, "index: MPS path currently supports exactly one index tensor");
      out = take(packed_indices.front());
    } else if (platform() == Platform::CUDA) {
      const bool native_enabled = native::cuda::native_indexing_enabled();
      const bool native_strict = native::cuda::native_indexing_strict();
      const bool route_supported = native::cuda::supports_index(*this, packed_indices);
      out = cuda_indexing_route_or_bridge_(
          "index",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return pluss::codem::native::cpu::index(*this, packed_indices); });
    } else {
      require_backend_dispatch_supported_(platform(), "index");
      out = pluss::codem::native::cpu::index(*this, packed_indices);
    }

    if (series_tape::recording_enabled() && series_tape::tracks_slope(*this)) {
      if (platform() == Platform::CPU) {
        bind_cpu_index_backward_(out, *this, packed_indices);
      } else if (platform() != Platform::MPS && supports_tracked_index_backward_(*this, packed_indices)) {
        bind_single_integer_index_backward_(out, *this, packed_indices.front());
      } else if (platform() != Platform::MPS) {
        PLUSS_FAIL(
            "index: reverse-mode sensitivity currently supports tracked paths only for one integer index tensor on dim0; "
            "use gather/take/select for other differentiable indexing patterns");
      }
    }
    return out;
  };

  const bool requested_all_advanced = std::all_of(
      requested.begin(), requested.end(), [](const ExtractSpec& s) { return s.kind == ExtractSpec::Kind::AdvancedIndex; });

  // Mixed advanced-index fast path (Torch-like intent):
  // route when every non-advanced spec is an identity SliceAll() and there is
  // at least one advanced index tensor. This avoids extract()->index_select
  // Cartesian blow-ups for patterns like:
  //   index({idx0, idx1, SliceAll()})
  bool has_advanced = false;
  bool mixed_adv_identity_only = true;
  std::int64_t dim_cursor = 0;
  std::vector<Series> full_indices(static_cast<std::size_t>(dim()), Series::undefined());
  for (const auto& s : normalized) {
    if (s.kind == ExtractSpec::Kind::NewAxis || s.kind == ExtractSpec::Kind::Ellipsis) {
      mixed_adv_identity_only = false;
      break;
    }
    PLUSS_CHECK(dim_cursor < dim(), "Series::index: internal dimension cursor overflow");
    if (s.kind == ExtractSpec::Kind::AdvancedIndex) {
      has_advanced = true;
      PLUSS_CHECK(s.advanced_impl, "Series::index: advanced index Series is undefined");
      Series idx;
      idx.impl() = s.advanced_impl;
      full_indices[static_cast<std::size_t>(dim_cursor)] = std::move(idx);
      ++dim_cursor;
      continue;
    }
    if (s.kind == ExtractSpec::Kind::Slice) {
      const auto b = normalize_slice_bounds_(s.slice, size(dim_cursor));
      if (!is_identity_slice_(b, size(dim_cursor))) {
        mixed_adv_identity_only = false;
        break;
      }
      ++dim_cursor;
      continue;
    }
    mixed_adv_identity_only = false;
    break;
  }
  if (mixed_adv_identity_only && has_advanced && dim_cursor == dim()) {
    if (platform() == Platform::CPU) {
      Series out = pluss::codem::native::cpu::index(*this, full_indices);
      if (series_tape::recording_enabled() && series_tape::tracks_slope(*this)) {
        bind_cpu_index_backward_(out, *this, full_indices);
      }
      return out;
    }
    if (platform() == Platform::CUDA) {
      for (const auto& idx : full_indices) {
        if (!idx.defined()) continue;
        require_same_platform(*this, idx, "index");
      }
      const bool native_enabled = native::cuda::native_indexing_enabled();
      const bool native_strict = native::cuda::native_indexing_strict();
      const bool route_supported =
          std::all_of(full_indices.begin(), full_indices.end(), [](const Series& i) { return i.defined(); }) &&
          native::cuda::supports_index(*this, full_indices);
      Series out = cuda_indexing_route_or_bridge_(
          "index",
          native_enabled,
          native_strict,
          route_supported,
          [&]() { return pluss::codem::native::cpu::index(*this, full_indices); });
      if (series_tape::recording_enabled() && series_tape::tracks_slope(*this)) {
        if (supports_tracked_index_backward_(*this, full_indices)) {
          bind_single_integer_index_backward_(out, *this, full_indices.front());
        } else if (!full_indices.empty()) {
          PLUSS_FAIL(
              "index: reverse-mode sensitivity currently supports tracked paths only for one integer index tensor on dim0; "
              "use gather/take/select for other differentiable indexing patterns");
        }
      }
      return out;
    }
    // MPS keeps the existing conservative path for mixed-index forms.
  }

  if (!requested_all_advanced) {
    return extract(specs);
  }

  return dispatch_advanced(pack_advanced_indices(
      std::span<const ExtractSpec>(requested.data(), requested.size())));
}
Series Series::take(const Series& index) const {
  require_same_platform(*this, index, "take");
  Series out;
  if (platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const bool native_supported = mps_take_fast_path_supported_(*this, index);
    if (strict_native && !native_supported) {
      require_mps_native_dispatch_supported_("take");
    }
    if (mps_native_take_put_enabled_() && native_supported) {
      const std::array<std::int64_t, 1> flat_shape{{numel()}};
      const std::array<std::int64_t, 1> out_flat_shape{{index.numel()}};
      const bool direct_lane = native::mps::supports_gather(scalar_type(), ScalarType::Int64);
      const ScalarType work_dt = direct_lane ? scalar_type() : ScalarType::Float32;
      const Series work_self =
          (scalar_type() == work_dt)
              ? this->contiguous()
              : this->to(work_dt).contiguous();
      const Series flat_self =
          work_self.reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
      const Series flat_idx = index.contiguous().reshape(std::span<const std::int64_t>(out_flat_shape.data(), out_flat_shape.size()));
      Series out_flat = Series::empty(
          std::span<const std::int64_t>(out_flat_shape.data(), out_flat_shape.size()),
          work_dt,
          Platform::MPS);
      native::mps::gather(flat_self, /*dim=*/0, flat_idx, out_flat);
      // Keep reshaped aliases coherent when MPS buffer cache entries are keyed by impl.
      (void)static_cast<const Series&>(out_flat).raw_data();
      // Materialize a stable value buffer so downstream autodiff links don't
      // depend on transient reshape aliases around MPS gather outputs.
      out = out_flat.reshape(index.sizes()).clone();
      if (work_dt != scalar_type()) {
        out = out.to(scalar_type());
      }
    } else {
      // Correctness-first fallback for unsupported dtype/index combinations.
      const Series cpu_self = this->to(Platform::CPU);
      const Series cpu_idx =
          (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).to(Platform::CPU);
      out = pluss::codem::native::cpu::take(cpu_self, cpu_idx).to(Platform::MPS);
    }
  } else if (platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_indexing_enabled();
    const bool native_strict = native::cuda::native_indexing_strict();
    const bool route_supported = native::cuda::supports_take(*this, index);
    out = cuda_indexing_route_or_bridge_(
        "take",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::codem::native::cpu::take(*this, index); });
  } else {
    require_backend_dispatch_supported_(platform(), "take");
    out = pluss::codem::native::cpu::take(*this, index);
  }

  const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  maybe_link_unary_op_(
      out,
      *this,
      "take",
      [input = *this, idx](const Series& out_sens) {
        const std::array<std::int64_t, 1> flat_index_shape{{idx.numel()}};
        const std::array<std::int64_t, 1> flat_input_shape{{input.numel()}};
        Series flat_idx =
            idx.reshape(std::span<const std::int64_t>(flat_index_shape.data(), flat_index_shape.size()));

        Series sens = out_sens;
        if (sens.scalar_type() != input.scalar_type()) sens = sens.to(input.scalar_type());
        if (sens.platform() != input.platform()) sens = sens.to(input.platform());
        const Series flat_sens =
            sens.reshape(std::span<const std::int64_t>(flat_index_shape.data(), flat_index_shape.size()));

        Series base_flat =
            Series::zeros(std::span<const std::int64_t>(flat_input_shape.data(), flat_input_shape.size()), input.scalar_type())
                .to(input.platform());
        Series grad_flat = ops_internal::scatter_add(base_flat, /*dim=*/0, flat_idx, flat_sens);
        return grad_flat.reshape(input.sizes());
      });
  return out;
}
Series Series::put_(const Series& index, const Series& source, bool accumulate) {
  guard_direct_inplace_value_op_("put_", *this);
  require_same_platform(*this, index, "put_");
  require_same_platform(*this, source, "put_");
  if (platform() == Platform::MPS) {
    const bool strict_native = !mps_cpu_fallback_enabled_();
    const bool native_supported = mps_put_fast_path_supported_(*this, index, source, accumulate);
    if (strict_native && !native_supported) {
      require_mps_native_dispatch_supported_("put_");
    }
    if (mps_native_take_put_enabled_() &&
        native_supported &&
        (source.numel() == 1 || source.numel() == index.numel())) {
      const std::array<std::int64_t, 1> flat_shape{{numel()}};
      const std::array<std::int64_t, 1> idx_shape{{index.numel()}};
      const Series flat_idx = index.contiguous().reshape(std::span<const std::int64_t>(idx_shape.data(), idx_shape.size()));
      const Series idx_cpu = flat_idx.to(Platform::CPU);
      const bool direct_lane =
          native::mps::supports_scatter(scalar_type(), ScalarType::Int64, scalar_type()) &&
          (!accumulate || native::mps::supports_scatter_reduce(scalar_type(), ScalarType::Int64, scalar_type()));
      const ScalarType work_dt = direct_lane ? scalar_type() : ScalarType::Float32;

      // scatter_set is not deterministic for duplicate writes; preserve CPU last-write semantics.
      if (!accumulate && has_duplicate_linear_indices_(idx_cpu, numel())) {
        const Series cpu_self = this->to(Platform::CPU);
        const Series cpu_idx =
            (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).to(Platform::CPU);
        const Series cpu_src = source.to(scalar_type()).to(Platform::CPU);
        const Series cpu_out = pluss::codem::native::cpu::put(cpu_self, cpu_idx, cpu_src, accumulate);
        copy_(cpu_out.to(Platform::MPS));
        return finish_direct_inplace_value_op_(*this);
      }

      const auto to_work_lane_mps = [&](const Series& s) {
        if (s.scalar_type() == work_dt) {
          return s.contiguous();
        }
        return s.to(work_dt).contiguous();
      };

      Series flat_src = to_work_lane_mps(source);
      if (flat_src.numel() == 1 && flat_idx.numel() != 1) {
        const std::array<std::int64_t, 1> src_expand{{flat_idx.numel()}};
        flat_src = flat_src.expand(std::span<const std::int64_t>(src_expand.data(), src_expand.size()), true).contiguous();
      } else {
        const std::array<std::int64_t, 1> src_shape{{flat_idx.numel()}};
        flat_src = flat_src.reshape(std::span<const std::int64_t>(src_shape.data(), src_shape.size())).contiguous();
      }

      Series flat_out = to_work_lane_mps(*this)
                            .reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()))
                            .clone();
      if (accumulate) {
        native::mps::scatter_reduce_(flat_out, /*dim=*/0, flat_idx, flat_src, native::mps::ScatterReduceOp::Sum);
      } else {
        native::mps::scatter_set_(flat_out, /*dim=*/0, flat_idx, flat_src);
      }
      // Keep reshaped aliases coherent when MPS buffer cache entries are keyed by impl.
      (void)static_cast<const Series&>(flat_out).raw_data();

      Series out = flat_out.reshape(sizes());
      if (work_dt != scalar_type()) {
        out = out.to(scalar_type());
      }
      copy_(out);
      return finish_direct_inplace_value_op_(*this);
    }

    // Correctness-first fallback for unsupported dtype/index/source combinations.
    const Series cpu_self = this->to(Platform::CPU);
    const Series cpu_idx =
        (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).to(Platform::CPU);
    const Series cpu_src = source.to(scalar_type()).to(Platform::CPU);
    const Series cpu_out = pluss::codem::native::cpu::put(cpu_self, cpu_idx, cpu_src, accumulate);
    copy_(cpu_out.to(Platform::MPS));
    return finish_direct_inplace_value_op_(*this);
  }
  if (platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_indexing_enabled();
    const bool native_strict = native::cuda::native_indexing_strict();
    const bool route_supported = native::cuda::supports_put(*this, index, source, accumulate);
    Series out = cuda_indexing_route_or_bridge_(
        "put_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::codem::native::cpu::put(*this, index, source, accumulate); });
    this->copy_(out);
    return finish_direct_inplace_value_op_(*this);
  }
  require_backend_dispatch_supported_(platform(), "put_");
  Series out = pluss::codem::native::cpu::put(*this, index, source, accumulate);
  this->copy_(out);
  return finish_direct_inplace_value_op_(*this);
}
Series Series::nonzero() const { return pluss::codem::ops_internal::nonzero(*this); }

Series Series::tan() const { return ops_internal::tan(*this); }
Series& Series::tan_() { guard_direct_inplace_value_op_("tan_", *this); *this = tan(); return finish_direct_inplace_value_op_(*this); }

Series Series::asin() const { return ops_internal::asin(*this); }
Series& Series::asin_() { guard_direct_inplace_value_op_("asin_", *this); *this = asin(); return finish_direct_inplace_value_op_(*this); }

Series Series::acos() const { return ops_internal::acos(*this); }
Series& Series::acos_() { guard_direct_inplace_value_op_("acos_", *this); *this = acos(); return finish_direct_inplace_value_op_(*this); }

Series Series::atan() const { return ops_internal::atan(*this); }
Series& Series::atan_() { guard_direct_inplace_value_op_("atan_", *this); *this = atan(); return finish_direct_inplace_value_op_(*this); }

Series Series::sinh() const { return ops_internal::sinh(*this); }
Series& Series::sinh_() { guard_direct_inplace_value_op_("sinh_", *this); *this = sinh(); return finish_direct_inplace_value_op_(*this); }

Series Series::cosh() const { return ops_internal::cosh(*this); }
Series& Series::cosh_() { guard_direct_inplace_value_op_("cosh_", *this); *this = cosh(); return finish_direct_inplace_value_op_(*this); }

Series Series::floor() const { return ops_internal::floor(*this); }
Series& Series::floor_() { guard_direct_inplace_value_op_("floor_", *this); *this = floor(); return finish_direct_inplace_value_op_(*this); }

Series Series::ceil() const { return ops_internal::ceil(*this); }
Series& Series::ceil_() { guard_direct_inplace_value_op_("ceil_", *this); *this = ceil(); return finish_direct_inplace_value_op_(*this); }

Series Series::round() const { return ops_internal::round(*this); }
Series& Series::round_() { guard_direct_inplace_value_op_("round_", *this); *this = round(); return finish_direct_inplace_value_op_(*this); }

Series Series::trunc() const { return ops_internal::trunc(*this); }
Series& Series::trunc_() { guard_direct_inplace_value_op_("trunc_", *this); *this = trunc(); return finish_direct_inplace_value_op_(*this); }

Series Series::reciprocal() const { return ops_internal::reciprocal(*this); }
Series& Series::reciprocal_() { guard_direct_inplace_value_op_("reciprocal_", *this); *this = reciprocal(); return finish_direct_inplace_value_op_(*this); }

Series Series::sigmoid() const { return ops_internal::sigmoid(*this); }
Series& Series::sigmoid_() { guard_direct_inplace_value_op_("sigmoid_", *this); *this = sigmoid(); return finish_direct_inplace_value_op_(*this); }

Series Series::relu() const { return ops_internal::relu(*this); }
Series& Series::relu_() { guard_direct_inplace_value_op_("relu_", *this); *this = relu(); return finish_direct_inplace_value_op_(*this); }

Series& Series::logical_not_() { guard_direct_inplace_value_op_("logical_not_", *this); *this = ops_internal::logical_not(*this); return finish_direct_inplace_value_op_(*this); }

Series Series::masked_select(const Series& mask) const {return pluss::codem::ops_internal::masked_select(*this, mask);}

Series Series::masked_scatter(const Series& mask, const Series& src) const {return pluss::codem::ops_internal::masked_scatter(*this, mask, src);}

Series& Series::masked_scatter_(const Series& mask, const Series& src) {
  guard_direct_inplace_value_op_("masked_scatter_", *this);
  pluss::codem::ops_internal::masked_scatter_(*this, mask, src);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::unique(bool sorted) const {return pluss::codem::ops_internal::unique(*this, sorted);}
Series Series::isin(const Series& test_elements) const {
  return pluss::codem::ops_internal::isin(*this, test_elements);
}
Series Series::isin(const Series& test_elements, bool assume_unique) const {
  return pluss::codem::ops_internal::isin(*this, test_elements, assume_unique);
}
Series Series::isin(const Series& test_elements, bool assume_unique, bool invert) const {
  return pluss::codem::ops_internal::isin(*this, test_elements, assume_unique, invert);
}
Series Series::union1d(const Series& other) const {
  return pluss::codem::ops_internal::union1d(*this, other);
}
Series Series::union1d(const Series& other, bool sorted) const {
  return pluss::codem::ops_internal::union1d(*this, other, sorted);
}
Series Series::intersect1d(const Series& other) const {
  return pluss::codem::ops_internal::intersect1d(*this, other);
}
Series Series::intersect1d(const Series& other, bool sorted) const {
  return pluss::codem::ops_internal::intersect1d(*this, other, sorted);
}
Series Series::intersect1d(const Series& other, bool sorted, bool assume_unique) const {
  return pluss::codem::ops_internal::intersect1d(*this, other, sorted, assume_unique);
}
Series Series::setdiff1d(const Series& other) const {
  return pluss::codem::ops_internal::setdiff1d(*this, other);
}
Series Series::setdiff1d(const Series& other, bool sorted) const {
  return pluss::codem::ops_internal::setdiff1d(*this, other, sorted);
}
Series Series::setdiff1d(const Series& other, bool sorted, bool assume_unique) const {
  return pluss::codem::ops_internal::setdiff1d(*this, other, sorted, assume_unique);
}
Series Series::setxor1d(const Series& other) const {
  return pluss::codem::ops_internal::setxor1d(*this, other);
}
Series Series::setxor1d(const Series& other, bool sorted) const {
  return pluss::codem::ops_internal::setxor1d(*this, other, sorted);
}
Series Series::setxor1d(const Series& other, bool sorted, bool assume_unique) const {
  return pluss::codem::ops_internal::setxor1d(*this, other, sorted, assume_unique);
}
Series Series::unionize(pluss::msharp::RangeView::Of<Series> others, bool sorted) const {
  return pluss::codem::ops_internal::set_unionize(*this, others, sorted);
}
Series Series::intersection(pluss::msharp::RangeView::Of<Series> others,
                            bool sorted,
                            bool assume_unique) const {
  return pluss::codem::ops_internal::set_intersection(*this, others, sorted, assume_unique);
}
Series Series::unionize_along(const Series& other, std::int64_t dim) const {
  return pluss::codem::ops_internal::unionize_along(*this, other, dim, /*sorted=*/true);
}
Series Series::unionize_along(const Series& other, std::int64_t dim, bool sorted) const {
  return pluss::codem::ops_internal::unionize_along(*this, other, dim, sorted);
}
Series Series::intersection_along(const Series& other, std::int64_t dim) const {
  return pluss::codem::ops_internal::intersection_along(
      *this,
      other,
      dim,
      /*sorted=*/true,
      /*assume_unique=*/false);
}
Series Series::intersection_along(const Series& other, std::int64_t dim, bool sorted) const {
  return pluss::codem::ops_internal::intersection_along(
      *this,
      other,
      dim,
      sorted,
      /*assume_unique=*/false);
}
Series Series::intersection_along(const Series& other,
                                  std::int64_t dim,
                                  bool sorted,
                                  bool assume_unique) const {
  return pluss::codem::ops_internal::intersection_along(*this, other, dim, sorted, assume_unique);
}
Series Series::setdiff_along(const Series& other, std::int64_t dim) const {
  return pluss::codem::ops_internal::setdiff_along(
      *this,
      other,
      dim,
      /*sorted=*/true,
      /*assume_unique=*/false);
}
Series Series::setdiff_along(const Series& other, std::int64_t dim, bool sorted) const {
  return pluss::codem::ops_internal::setdiff_along(
      *this,
      other,
      dim,
      sorted,
      /*assume_unique=*/false);
}
Series Series::setdiff_along(const Series& other,
                             std::int64_t dim,
                             bool sorted,
                             bool assume_unique) const {
  return pluss::codem::ops_internal::setdiff_along(*this, other, dim, sorted, assume_unique);
}
Series Series::setxor_along(const Series& other, std::int64_t dim) const {
  return pluss::codem::ops_internal::setxor_along(
      *this,
      other,
      dim,
      /*sorted=*/true,
      /*assume_unique=*/false);
}
Series Series::setxor_along(const Series& other, std::int64_t dim, bool sorted) const {
  return pluss::codem::ops_internal::setxor_along(
      *this,
      other,
      dim,
      sorted,
      /*assume_unique=*/false);
}
Series Series::setxor_along(const Series& other,
                            std::int64_t dim,
                            bool sorted,
                            bool assume_unique) const {
  return pluss::codem::ops_internal::setxor_along(*this, other, dim, sorted, assume_unique);
}
Series Series::comp(const Series& universe) const {
  return pluss::codem::ops_internal::setdiff1d(
      universe,
      *this,
      /*sorted=*/false,
      /*assume_unique=*/false);
}
Series Series::comp(const Series& universe, bool sorted) const {
  return pluss::codem::ops_internal::setdiff1d(
      universe,
      *this,
      sorted,
      /*assume_unique=*/false);
}
Series Series::comp(const Series& universe, bool sorted, bool assume_unique) const {
  return pluss::codem::ops_internal::setdiff1d(universe, *this, sorted, assume_unique);
}
Series Series::cartprod(const Series& other) const {
  return cartprod_pair_1d_(*this, other);
}
Series Series::cartprod(pluss::msharp::RangeView::Of<Series> others) const {
  Series base = set_flatten_1d_(*this);
  Series out = base.reshape({base.numel(), 1});
  for (const Series& other : others) {
    require_same_platform(out, other, "cartprod");
    PLUSS_CHECK(out.scalar_type() == other.scalar_type(), "cartprod: input dtypes must match");
    const Series rhs = set_flatten_1d_(other);
    const std::int64_t left_rows = out.size(0);
    const std::int64_t cols = out.size(1);
    const std::int64_t m = rhs.numel();
    if (left_rows == 0 || m == 0) {
      out = Series::empty(std::vector<std::int64_t>{0, cols + 1}, out.scalar_type(), out.platform());
      continue;
    }
    const Series left = out.repeat_interleave(m, /*dim=*/0);
    const Series right = rhs.repeat(std::vector<std::int64_t>{left_rows}).reshape({left_rows * m, 1});
    out = pluss::unite({left, right}, /*dim=*/1, /*strict_dtype=*/true);
  }
  return out;
}
Series Series::is_prime() const {
  return is_prime_series_fast_(*this);
}
Series Series::prime() const {
  return nth_prime_series_fast_(*this);
}
Series Series::factor() const {
  return least_prime_factor_series_fast_(*this);
}

Series Series::bincount(std::int64_t minlength) const {return pluss::codem::ops_internal::bincount(*this, minlength);}

Series Series::histc(std::int64_t bins, double min, double max) const {return pluss::codem::ops_internal::histc(*this, bins, min, max);}

Series Series::log1p() const {
  PLUSS_CALL_GUARD();
  return ops_internal::log1p(*this);
}
Series& Series::log1p_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("log1p_", *this);
  *this = log1p();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::expm1() const {
  PLUSS_CALL_GUARD();
  return ops_internal::expm1(*this);
}
Series& Series::expm1_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("expm1_", *this);
  *this = expm1();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::erf() const {
  PLUSS_CALL_GUARD();
  return ops_internal::erf(*this);
}
Series& Series::erf_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("erf_", *this);
  *this = erf();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::erfc() const {
  PLUSS_CALL_GUARD();
  return ops_internal::erfc(*this);
}
Series& Series::erfc_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("erfc_", *this);
  *this = erfc();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::lgamma() const {
  PLUSS_CALL_GUARD();
  return ops_internal::lgamma(*this);
}
Series& Series::lgamma_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("lgamma_", *this);
  *this = lgamma();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::asinh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::asinh(*this);
}
Series& Series::asinh_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("asinh_", *this);
  *this = asinh();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::acosh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::acosh(*this);
}
Series& Series::acosh_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("acosh_", *this);
  *this = acosh();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::atanh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::atanh(*this);
}
Series& Series::atanh_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("atanh_", *this);
  *this = atanh();
  return finish_direct_inplace_value_op_(*this);
}

Series Series::isnan() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isnan(*this);
}
Series& Series::isnan_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("isnan_", *this);
  Series out = isnan().to(scalar_type());
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::isinf() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isinf(*this);
}
Series& Series::isinf_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("isinf_", *this);
  Series out = isinf().to(scalar_type());
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::isfinite() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isfinite(*this);
}
Series& Series::isfinite_() {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("isfinite_", *this);
  Series out = isfinite().to(scalar_type());
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::sum(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::sum(*this, dim, keepdim);
}

Series Series::mean(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::mean(*this, dim, keepdim);
}

Series Series::amax(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::amax(*this, dim, keepdim);
}

::pluss::StackArray<Series> Series::max_pair(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ::pluss::StackArray<Series>{max(dim, keepdim), argmax(dim, keepdim)};
}

::pluss::StackArray<Series> Series::min_pair(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ::pluss::StackArray<Series>{min(dim, keepdim), argmin(dim, keepdim)};
}

::pluss::StackArray<Series> Series::argmax_pair(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return max_pair(dim, keepdim);
}

::pluss::StackArray<Series> Series::argmin_pair(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return min_pair(dim, keepdim);
}

::pluss::StackArray<Series> Series::sort_pair(std::int64_t dim, bool descending, bool stable) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ::pluss::StackArray<Series>{sort(dim, descending, stable), argsort(dim, descending, stable)};
}

::pluss::StackArray<Series> Series::topk_pair(
    std::int64_t k, std::int64_t dim, bool largest, bool sorted) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  PLUSS_CHECK(sorted, "topk_pair: sorted=false is not supported yet");

  const auto nd = this->dim();
  std::int64_t d = dim;
  if (d < 0) d += nd;
  PLUSS_CHECK(d >= 0 && d < nd, "topk_pair: dim out of range");
  const std::int64_t n = size(d);
  const std::int64_t k_eff = std::clamp<std::int64_t>(k, 0, n);

  Series values = topk(k, d, largest, sorted);
  Series order = argsort(d, largest, false);
  Series head = Series::arange(
                    0.0,
                    static_cast<double>(k_eff),
                    1.0,
                    ScalarType::Int64)
                    .to(platform(), ScalarType::Int64);
  Series indices = order.index_select(d, head);
  return ::pluss::StackArray<Series>{values, indices};
}

Series Series::amin(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::amin(*this, dim, keepdim);
}

Series Series::all(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::all(*this, dim, keepdim);
}

Series Series::any(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::any(*this, dim, keepdim);
}

Series Series::mean() const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::mean(*this);
}

Series Series::where(const Series& condition, const Series& other) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  // Support both styles:
  // - cond.where(a, b): torch-like (self is condition)
  // - a.where(cond, b): CodeM legacy (true selects other)
  if (scalar_type() == ScalarType::Bool && condition.scalar_type() != ScalarType::Bool) {
    return ops_internal::where(*this, condition, other);
  }
  return ops_internal::where(condition, other, *this);
}

Series& Series::where_(const Series& condition, const Series& other) {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("where_", *this);
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = where(condition, other);
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::clamp_min(double min) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::clamp_min(*this, min);
}

Series& Series::clamp_min_(double min) {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("clamp_min_", *this);
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_min(min);
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::clamp_max(double max) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::clamp_max(*this, max);
}

Series& Series::clamp_max_(double max) {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("clamp_max_", *this);
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_max(max);
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::nan_to_num(double nan, double posinf, double neginf) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::nan_to_num(*this, nan, posinf, neginf);
}

Series& Series::nan_to_num_(double nan, double posinf, double neginf) {
  PLUSS_CALL_GUARD();
  guard_direct_inplace_value_op_("nan_to_num_", *this);
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = nan_to_num(nan, posinf, neginf);
  copy_(out);
  return finish_direct_inplace_value_op_(*this);
}

Series Series::cumsum(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  (void)keepdim; // same shape output; keepdim for API parity
  Series a = is_contiguous() ? *this : contiguous();
  if (platform() == Platform::MPS && native::mps::supports_cumsum_dim(a.scalar_type())) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
    native::mps::cumsum_dim(a, dim, out);
    maybe_link_unary_op_(out, *this, "cumsum", [dim](const Series& out_sens) {
      return reverse_dim_(reverse_dim_(out_sens, dim).cumsum(dim), dim);
    });
    return out;
  }
  if (platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scan_enabled();
    const bool native_strict = native::cuda::native_scan_strict();
    const bool route_supported = native::cuda::supports_cumsum_scan(a, dim);
    return cuda_scan_route_or_bridge_(
        "cumsum",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
          native::cpu::cumsum_dim(a, dim, out);
          maybe_link_unary_op_(out, *this, "cumsum", [dim](const Series& out_sens) {
            return reverse_dim_(reverse_dim_(out_sens, dim).cumsum(dim), dim);
          });
          return out;
        });
  }
  require_backend_dispatch_supported_(platform(), "cumsum");
  Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
  native::cpu::cumsum_dim(a, dim, out);
  maybe_link_unary_op_(out, *this, "cumsum", [dim](const Series& out_sens) {
    return reverse_dim_(reverse_dim_(out_sens, dim).cumsum(dim), dim);
  });
  return out;
}



Series& Series::cumsum_(std::int64_t dim, bool keepdim) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = cumsum(dim, keepdim);
  copy_(out);
  return *this;
}



Series Series::cumprod(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  (void)keepdim;
  Series a = is_contiguous() ? *this : contiguous();
  if (platform() == Platform::MPS && native::mps::supports_cumprod_dim(a.scalar_type())) {
    Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
    native::mps::cumprod_dim(a, dim, out);
    return out;
  }
  if (platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_scan_enabled();
    const bool native_strict = native::cuda::native_scan_strict();
    const bool route_supported = native::cuda::supports_cumprod_scan(a, dim);
    return cuda_scan_route_or_bridge_(
        "cumprod",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
          native::cpu::cumprod_dim(a, dim, out);
          return out;
        });
  }
  require_backend_dispatch_supported_(platform(), "cumprod");
  Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
  native::cpu::cumprod_dim(a, dim, out);
  return out;
}



Series& Series::cumprod_(std::int64_t dim, bool keepdim) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = cumprod(dim, keepdim);
  copy_(out);
  return *this;
}


Series Series::where(const Series& condition, double other) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series b = Series::full({}, other, scalar_type()).expand_as(*this);
  return where(condition, b);
}
Series& Series::where_(const Series& condition, double other) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = where(condition, other);
  copy_(out);
  return *this;
}

Series Series::clamp_min(const Series& min) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  // broadcast min against self
  Series m = (min.scalar_type()==scalar_type()) ? min : min.to(scalar_type());
  Series cond = this->ge(m);
  return this->where(cond, m);
}
Series& Series::clamp_min_(const Series& min) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_min(min);
  copy_(out);
  return *this;
}


Series Series::clamp_max(const Series& max) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series m = (max.scalar_type()==scalar_type()) ? max : max.to(scalar_type());
  Series cond = this->le(m);
  return this->where(cond, m);
}
Series& Series::clamp_max_(const Series& max) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_max(max);
  copy_(out);
  return *this;
}


Series Series::scatter_reduce_sum(std::int64_t dim, const Series& index, const Series& src, bool include_self) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "sum", include_self);
}
Series Series::scatter_reduce_amax(std::int64_t dim, const Series& index, const Series& src, bool include_self) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "amax", include_self);
}
Series Series::scatter_reduce_amin(std::int64_t dim, const Series& index, const Series& src, bool include_self) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "amin", include_self);
}
Series Series::scatter_reduce_sum(std::int64_t dim, const Series& index, const Series& src, bool include_self, double initial_scalar) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "sum", include_self, true, initial_scalar);
}
Series Series::scatter_reduce_amax(std::int64_t dim, const Series& index, const Series& src, bool include_self, double initial_scalar) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "amax", include_self, true, initial_scalar);
}
Series Series::scatter_reduce_amin(std::int64_t dim, const Series& index, const Series& src, bool include_self, double initial_scalar) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::scatter_reduce(*this, dim, index, src, "amin", include_self, true, initial_scalar);
}

} // namespace pluss::codem::ops_internal
