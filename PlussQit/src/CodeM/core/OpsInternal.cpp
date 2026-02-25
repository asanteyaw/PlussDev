#include "CodeM/core/OpsInternal.h"

// OpsInternal.cpp defines Series member functions; it requires the full Series type.
#include "CodeM/core/Series.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
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
#include "CodeM/native/mps/Gather.h"
#include "CodeM/native/mps/MPSRuntime.h"
#include "CodeM/native/mps/Cast.h"
#include "CodeM/native/mps/Cum.h"
#include "CodeM/native/mps/Reductions.h"
#include "CodeM/native/mps/Scatter.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/Half.h"

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

void require_mps_native_dispatch_supported_(std::string_view op) {
  throw_error(
      std::string(op) +
      ": no native MPS kernel yet for this dtype/op. "
      "CPU fallback is disabled by policy.");
}

void require_backend_dispatch_supported_(Platform platform, std::string_view op) {
  switch (platform) {
    case Platform::CPU:
      return;
    case Platform::MPS:
      require_mps_native_dispatch_supported_(op);
      return;
    case Platform::CUDA:
      throw_error(
          std::string(op) +
          ": no native CUDA kernel yet. CPU fallback is not enabled for CUDA backend.");
    default:
      throw_error(std::string(op) + ": unsupported backend platform");
  }
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
  throw_error(
      std::string(op) + ": expected all Series on same platform, got " +
      platform_name_(a.platform()) + " and " + platform_name_(b.platform()));
}

static std::vector<std::int64_t> broadcast_out_shape_(std::span<const std::int64_t> a,
                                                      std::span<const std::int64_t> b) {
  return SeriesIterator::broadcast_shape(a, b);
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
  ScalarType dt = promote_types(a0.scalar_type(), b0.scalar_type());
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
    PLUSS_CHECK(native::mps::supports_binary(op, ScalarType::Float32),
                "binary op: MPS path does not yet support this dtype/op combination");
    const Series aa = a.to(ScalarType::Float32).contiguous();
    const Series bb = b.to(ScalarType::Float32).contiguous();
    Series out_f = Series::empty(out_shape, ScalarType::Float32, a.platform());
    native::mps::binary(op, aa, bb, out_f);
    return out_f.to(dt);
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
    PLUSS_CHECK(native::mps::supports_binary(op, ScalarType::Float32),
                "comparison op: MPS path does not yet support this dtype/op combination");
    const Series aa = a.to(ScalarType::Float32).contiguous();
    const Series bb = b.to(ScalarType::Float32).contiguous();
    native::mps::binary(op, aa, bb, out);
    return out;
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
    PLUSS_CHECK(native::mps::supports_unary(op, ScalarType::Float32),
                "unary op: MPS path does not yet support this dtype/op combination");
    const Series aa = a.to(ScalarType::Float32).contiguous();
    Series out_f = Series::empty(a.sizes(), ScalarType::Float32, a.platform());
    native::mps::unary(op, aa, out_f);
    return out_f.to(a.scalar_type());
  }
  require_backend_dispatch_supported_(a.platform(), "unary op");
  auto fn = native::cpu::lookup_unary(op, a.scalar_type());
  PLUSS_CHECK(fn != nullptr, "unary kernel missing");
  fn(a, out);
  return out;
}

template <typename CpuUnaryFn>
Series unary_cpu_with_platform_dispatch_policy_(const Series& a, std::string_view op_name, CpuUnaryFn cpu_fn) {
  require_backend_dispatch_supported_(a.platform(), op_name);
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  cpu_fn(a, out);
  return out;
}

static bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  return a.sizes().size() == b.sizes().size() &&
         std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin());
}

static Series reduce_sensitivity_to_like_(const Series& sens, const Series& like, const char* op) {
  if (same_shape_(sens, like)) return sens;
  if (like.numel() == 1) return sens.sum();
  throw_error(std::string(op) + ": unsupported sensitivity reduction for broadcasted shape");
  return sens;
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
  link->parents = {a, b};
  link->propagate = [a, b, ta, tb, op, lhs_rule = std::move(lhs_rule), rhs_rule = std::move(rhs_rule)](const Series& out_sens) {
    if (ta) {
      const Series ga = reduce_sensitivity_to_like_(lhs_rule(out_sens), a, op);
      series_tape::accumulate_sensitivity(a, ga);
    }
    if (tb) {
      const Series gb = reduce_sensitivity_to_like_(rhs_rule(out_sens), b, op);
      series_tape::accumulate_sensitivity(b, gb);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

Series add(const Series& a, const Series& b) {
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
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
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Sub, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "sub",
      [](const Series& g) { return g; },
      [](const Series& g) { return g.neg(); });
  return out;
}
Series mul(const Series& a, const Series& b) {
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
  auto out = binary_op_(native::cpu::BinaryOpId::Mul, aa, bb);
  maybe_link_binary_op_(
      out, a, b, "mul",
      [b](const Series& g) { return g.mul(b); },
      [a](const Series& g) { return g.mul(a); });
  return out;
}
Series div(const Series& a, const Series& b) {
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
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
  const auto out_dt = pluss::codem::promote_types(a.scalar_type(), b.scalar_type());
  auto aa = (a.scalar_type() == out_dt) ? a : a.to(out_dt);
  auto bb = (b.scalar_type() == out_dt) ? b : b.to(out_dt);
 return binary_op_(native::cpu::BinaryOpId::Pow, aa, bb); }

static Series scalar_like_(const Series& a, double s) {
  const std::array<std::int64_t, 1> one{{1}};
  return Series::full(std::span<const std::int64_t>(one.data(), one.size()), s, a.scalar_type())
      .to(a.platform());
}

Series add(const Series& a, double s) { return add(a, scalar_like_(a, s)); }
Series sub(const Series& a, double s) { return sub(a, scalar_like_(a, s)); }
Series mul(const Series& a, double s) { return mul(a, scalar_like_(a, s)); }
Series div(const Series& a, double s) { return div(a, scalar_like_(a, s)); }
Series pow(const Series& a, double s) { return pow(a, scalar_like_(a, s)); }

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
  return unary_op_(native::cpu::UnaryOpId::Abs, a);
}
Series& abs_(Series& a) { a = abs(a); return a; }
Series neg(const Series& a) {
  auto out = unary_op_(native::cpu::UnaryOpId::Neg, a);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    auto link = std::make_shared<series_tape::SourceLink>();
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
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Sqrt, a.scalar_type()),
                "sqrt: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Sqrt, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "sqrt");
  native::cpu::sqrt(a, out);
  return out;
}
Series exp(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Exp, a.scalar_type()),
                "exp: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Exp, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "exp");
  native::cpu::exp(a, out);
  return out;
}
Series log(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Log, a.scalar_type()),
                "log: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Log, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "log");
  native::cpu::log(a, out);
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
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Floor, a.scalar_type()),
                "floor: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Floor, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "floor");
  native::cpu::floor(a, out);
  return out;
}
Series ceil(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Ceil, a.scalar_type()),
                "ceil: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Ceil, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "ceil");
  native::cpu::ceil(a, out);
  return out;
}
Series round(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Round, a.scalar_type()),
                "round: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Round, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "round");
  native::cpu::round(a, out);
  return out;
}
Series trunc(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Trunc, a.scalar_type()),
                "trunc: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Trunc, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "trunc");
  native::cpu::trunc(a, out);
  return out;
}
Series isnan(const Series& a) {
  Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsNan, a.scalar_type()),
                "isnan: MPS path does not yet support this dtype");
    native::mps::unary_pred(native::mps::UnaryPredOp::IsNan, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "isnan");
  native::cpu::isnan(a, out);
  return out;
}
Series isinf(const Series& a) {
  Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsInf, a.scalar_type()),
                "isinf: MPS path does not yet support this dtype");
    native::mps::unary_pred(native::mps::UnaryPredOp::IsInf, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "isinf");
  native::cpu::isinf(a, out);
  return out;
}

Series sin(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Sin, a.scalar_type()),
                "sin: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Sin, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "sin");
  native::cpu::sin(a, out);
  return out;
}
Series cos(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Cos, a.scalar_type()),
                "cos: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Cos, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "cos");
  native::cpu::cos(a, out);
  return out;
}
Series tanh(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Tanh, a.scalar_type()),
                "tanh: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Tanh, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "tanh");
  native::cpu::tanh(a, out);
  return out;
}

Series tan(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Tan, a.scalar_type()),
                "tan: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Tan, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "tan");
  native::cpu::tan(a, out);
  return out;
}
Series asin(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Asin, a.scalar_type()),
                "asin: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Asin, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "asin");
  native::cpu::asin(a, out);
  return out;
}
Series acos(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Acos, a.scalar_type()),
                "acos: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Acos, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "acos");
  native::cpu::acos(a, out);
  return out;
}
Series atan(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Atan, a.scalar_type()),
                "atan: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Atan, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "atan");
  native::cpu::atan(a, out);
  return out;
}
Series sinh(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Sinh, a.scalar_type()),
                "sinh: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Sinh, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "sinh");
  native::cpu::sinh(a, out);
  return out;
}
Series cosh(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Cosh, a.scalar_type()),
                "cosh: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Cosh, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "cosh");
  native::cpu::cosh(a, out);
  return out;
}

Series log1p(const Series& a) { return unary_op_(native::cpu::UnaryOpId::Log1p, a); }
Series expm1(const Series& a) { return unary_op_(native::cpu::UnaryOpId::Expm1, a); }
Series reciprocal(const Series& a) {
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Reciprocal, a.scalar_type()),
                "reciprocal: MPS path does not yet support this dtype");
    native::mps::unary_math(native::mps::UnaryMathOp::Reciprocal, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "reciprocal");
  native::cpu::reciprocal(a, out);
  return out;
}

Series erf(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    const auto one = scalar_like_(x, 1.0);
    const auto neg_one = scalar_like_(x, -1.0);
    const auto p = scalar_like_(x, 0.3275911);
    const auto a1 = scalar_like_(x, 0.254829592);
    const auto a2 = scalar_like_(x, -0.284496736);
    const auto a3 = scalar_like_(x, 1.421413741);
    const auto a4 = scalar_like_(x, -1.453152027);
    const auto a5 = scalar_like_(x, 1.061405429);
    const auto ax = abs(x);
    const auto t = reciprocal(add(one, mul(p, ax)));
    const auto poly = mul(
        t,
        add(a1, mul(t, add(a2, mul(t, add(a3, mul(t, add(a4, mul(t, a5)))))))));
    const auto y = sub(one, mul(poly, exp(neg(mul(ax, ax)))));
    const auto sign = where(lt(x, scalar_like_(x, 0.0)), neg_one, one);
    return mul(sign, y).to(a.scalar_type());
  }
  return unary_op_(native::cpu::UnaryOpId::Erf, a);
}
Series erfc(const Series& a) {
  if (a.platform() == Platform::MPS) {
    const auto x = a.contiguous().to(ScalarType::Float32);
    return sub(scalar_like_(x, 1.0), erf(x)).to(a.scalar_type());
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
  Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_pred(native::mps::UnaryPredOp::IsFinite, a.scalar_type()),
                "isfinite: MPS path does not yet support this dtype");
    native::mps::unary_pred(native::mps::UnaryPredOp::IsFinite, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "isfinite");
  native::cpu::isfinite(a, out);
  return out;
}

Series logical_not(const Series& a) {
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(a.scalar_type() == ScalarType::Bool, "logical_not: expects bool tensor on MPS");
    const Series af = a.to(ScalarType::Float32).contiguous();
    return eq(af, scalar_like_(af, 0.0));
  }
  require_backend_dispatch_supported_(a.platform(), "logical_not");
  Series out = Series::empty(a.sizes(), ScalarType::Bool, a.platform());
  native::cpu::logical_not(a, out);
  return out;
}

Series logical_and(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_and: undefined");
  require_same_platform(a, b, "logical_and");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_and: expects bool tensors");
  if (a.platform() == Platform::MPS) {
    const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
    const Series af = a.expand(shape, true).to(ScalarType::Float32).contiguous();
    const Series bf = b.expand(shape, true).to(ScalarType::Float32).contiguous();
    return gt(mul(af, bf), scalar_like_(af, 0.5));
  }
  require_backend_dispatch_supported_(a.platform(), "logical_and");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = a.expand(shape, true).contiguous();
  auto bb = b.expand(shape, true).contiguous();
  Series out = Series::empty(shape, ScalarType::Bool, a.platform());
  auto* op = reinterpret_cast<bool*>(const_cast<std::byte*>(out.raw_data()));
  const bool* ap = reinterpret_cast<const bool*>(aa.raw_data());
  const bool* bp = reinterpret_cast<const bool*>(bb.raw_data());
  const auto n = out.numel();
  for (std::int64_t i=0;i<n;++i) op[i] = ap[i] && bp[i];
  return out;
}

Series logical_or(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_or: undefined");
  require_same_platform(a, b, "logical_or");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_or: expects bool tensors");
  if (a.platform() == Platform::MPS) {
    const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
    const Series af = a.expand(shape, true).to(ScalarType::Float32).contiguous();
    const Series bf = b.expand(shape, true).to(ScalarType::Float32).contiguous();
    return gt(add(af, bf), scalar_like_(af, 0.0));
  }
  require_backend_dispatch_supported_(a.platform(), "logical_or");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = a.expand(shape, true).contiguous();
  auto bb = b.expand(shape, true).contiguous();
  Series out = Series::empty(shape, ScalarType::Bool, a.platform());
  auto* op = reinterpret_cast<bool*>(const_cast<std::byte*>(out.raw_data()));
  const bool* ap = reinterpret_cast<const bool*>(aa.raw_data());
  const bool* bp = reinterpret_cast<const bool*>(bb.raw_data());
  const auto n = out.numel();
  for (std::int64_t i=0;i<n;++i) op[i] = ap[i] || bp[i];
  return out;
}

Series logical_xor(const Series& a, const Series& b) {
  PLUSS_CHECK(a.defined() && b.defined(), "logical_xor: undefined");
  require_same_platform(a, b, "logical_xor");
  PLUSS_CHECK(a.scalar_type() == ScalarType::Bool && b.scalar_type() == ScalarType::Bool, "logical_xor: expects bool tensors");
  if (a.platform() == Platform::MPS) {
    const auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
    const Series af = a.expand(shape, true).to(ScalarType::Float32).contiguous();
    const Series bf = b.expand(shape, true).to(ScalarType::Float32).contiguous();
    return gt(abs(sub(af, bf)), scalar_like_(af, 0.5));
  }
  require_backend_dispatch_supported_(a.platform(), "logical_xor");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = a.expand(shape, true).contiguous();
  auto bb = b.expand(shape, true).contiguous();
  Series out = Series::empty(shape, ScalarType::Bool, a.platform());
  auto* op = reinterpret_cast<bool*>(const_cast<std::byte*>(out.raw_data()));
  const bool* ap = reinterpret_cast<const bool*>(aa.raw_data());
  const bool* bp = reinterpret_cast<const bool*>(bb.raw_data());
  const auto n = out.numel();
  for (std::int64_t i=0;i<n;++i) op[i] = (ap[i] != bp[i]);
  return out;
}

Series masked_select(const Series& a, const Series& mask) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "masked_select: mask must be bool");
  require_same_platform(a, mask, "masked_select");
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
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
    const Series idx = nz2d.squeeze(-1).contiguous();
    return gather(aa_flat, 0, idx);
  }
  require_backend_dispatch_supported_(a.platform(), "masked_select");
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
  for (std::int64_t i=0;i<n;++i) if (mp[i]) ++count;

  const std::array<std::int64_t, 1> out_sizes{count};
  Series out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), aa.scalar_type(), a.platform());
  const auto item = aa.dtype().itemsize;
  const std::byte* ap = aa.raw_data();
  auto* op = reinterpret_cast<std::byte*>(out.raw_mutable_data());
  std::int64_t j = 0;
  for (std::int64_t i=0;i<n;++i) {
    if (!mp[i]) continue;
    std::memcpy(op + j*item, ap + i*item, (std::size_t)item);
    ++j;
  }
  return out;
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
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const Series mm = mask.expand(shape, true).contiguous();
    const Series vv = value.to(a.scalar_type()).expand(shape, true).contiguous();
    a = where(mm, vv, a);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "masked_fill_");
  PLUSS_CHECK(a.is_contiguous(), "masked_fill_: in-place requires a contiguous Series");
  auto mm = mask.expand(shape, true).contiguous();
  const bool* mp = reinterpret_cast<const bool*>(mm.raw_data());

  // Value cast
  Series vv = value.to(a.scalar_type()).contiguous();
  const auto item = a.dtype().itemsize;
  const std::byte* vp = vv.raw_data();

  auto* ap = reinterpret_cast<std::byte*>(a.raw_mutable_data());
  const auto n = a.numel();
  for (std::int64_t i=0;i<n;++i) {
    if (!mp[i]) continue;
    std::memcpy(ap + i*item, vp, (std::size_t)item);
  }
  return a;
}

Series& masked_fill_(Series& a, const Series& mask, double value) {
  const std::array<std::int64_t, 1> one{1};
  auto v = Series::full(std::span<const std::int64_t>(one.data(), one.size()), value, ScalarType::Float64)
               .to(a.platform())
               .to(a.scalar_type());
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

Series sum(const Series& a) {
  PLUSS_CHECK(a.defined(), "sum: undefined");
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_sum_dim(aa.scalar_type())) {
      Series cur = aa;
      for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
        cur = sum(cur, d, false);
      }
      return cur;
    }
    require_backend_dispatch_supported_(aa.platform(), "sum");
  }
  Series out = Series::empty({}, aa.scalar_type(), aa.platform());
  native::cpu::sum_all(aa, out);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    auto link = std::make_shared<series_tape::SourceLink>();
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
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, /*keepdim=*/false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_sum_dim(aa.scalar_type())) {
      native::mps::sum_dim(aa, dim, out);
      auto reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
      if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
        const auto norm_dim = dim;
        auto link = std::make_shared<series_tape::SourceLink>();
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
    require_backend_dispatch_supported_(aa.platform(), "sum(dim)");
  }
  native::cpu::sum_dim(aa, dim, out);
  auto reduced = maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(a)) {
    const auto norm_dim = dim;
    auto link = std::make_shared<series_tape::SourceLink>();
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
  if (a.dim() == 0) return a.clone();
  Series cur = a.contiguous();
  if (cur.platform() != Platform::CPU) {
    if (cur.platform() == Platform::MPS && native::mps::supports_prod_dim(cur.scalar_type())) {
      for (std::int64_t d = static_cast<std::int64_t>(cur.dim()) - 1; d >= 0; --d) {
        cur = prod(cur, d, false);
      }
      return cur;
    }
    require_backend_dispatch_supported_(cur.platform(), "prod");
  }
  // prod_all CPU path: reduce sequentially using dim kernel.
  for (std::int64_t d = (std::int64_t)cur.dim()-1; d>=0; --d) cur = prod(cur, d, false);
  return cur;
}

Series prod(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "prod: undefined");
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
  Series out = Series::empty(out_sizes, aa.scalar_type(), aa.platform());
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_prod_dim(aa.scalar_type())) {
      native::mps::prod_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    require_backend_dispatch_supported_(aa.platform(), "prod(dim)");
  }
  native::cpu::prod_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series mean(const Series& a) {
  PLUSS_CHECK(a.defined(), "mean: undefined");
  if (a.dim() == 0) return a.clone();
  auto aa = a.contiguous();
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_sum_dim(aa.scalar_type())) {
      Series s = sum(aa);
      return div(s, scalar_like_(s, static_cast<double>(aa.numel())));
    }
    require_backend_dispatch_supported_(aa.platform(), "mean");
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
  if (a.dim() == 0) return a.clone();
  // mean = sum / reduce_n, for float types; for integral, promote to float32
  Series aa = a.contiguous();
  const auto nd = aa.dim();
  if (dim < 0) dim += nd;
  const auto reduce_n = aa.sizes()[(std::size_t)dim];
  if (aa.platform() != Platform::CPU) {
    if (aa.platform() == Platform::MPS && native::mps::supports_sum_dim(aa.scalar_type())) {
      Series s = sum(aa, dim, keepdim);
      return div(s, scalar_like_(s, static_cast<double>(reduce_n)));
    }
    require_backend_dispatch_supported_(aa.platform(), "mean(dim)");
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
  if (a.platform() != Platform::CPU &&
      !(a.platform() == Platform::MPS && native::mps::supports_amax_amin_dim(a.scalar_type()))) {
    require_backend_dispatch_supported_(a.platform(), "amax");
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
    if (aa.platform() == Platform::MPS && native::mps::supports_amax_amin_dim(aa.scalar_type())) {
      native::mps::amax_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    require_backend_dispatch_supported_(aa.platform(), "amax(dim)");
  }
  native::cpu::amax_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series amin(const Series& a) {
  PLUSS_CHECK(a.defined(), "amin: undefined");
  if (a.platform() != Platform::CPU &&
      !(a.platform() == Platform::MPS && native::mps::supports_amax_amin_dim(a.scalar_type()))) {
    require_backend_dispatch_supported_(a.platform(), "amin");
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
    if (aa.platform() == Platform::MPS && native::mps::supports_amax_amin_dim(aa.scalar_type())) {
      native::mps::amin_dim(aa, dim, out);
      return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
    }
    require_backend_dispatch_supported_(aa.platform(), "amin(dim)");
  }
  native::cpu::amin_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
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
    require_backend_dispatch_supported_(aa.platform(), "all(dim)");
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
    require_backend_dispatch_supported_(aa.platform(), "any(dim)");
  }
  native::cpu::any_dim(aa, dim, out);
  return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
}

Series argmax(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "argmax: undefined");
  if (a.platform() == Platform::MPS && native::mps::supports_argmax_argmin_dim(a.scalar_type())) {
    if (a.dim() == 0) {
      return Series::full({}, 0.0, ScalarType::Int64).to(a.platform());
    }
    auto aa = a.contiguous();
    const auto nd = aa.dim();
    if (dim < 0) dim += nd;
    auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
    Series out = Series::empty(out_sizes, ScalarType::Int64, aa.platform());
    native::mps::argmax_dim(aa, dim, out);
    return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  }
  require_backend_dispatch_supported_(a.platform(), "argmax");
  return native::cpu::argmax(a, dim, keepdim);
}
Series argmin(const Series& a, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(a.defined(), "argmin: undefined");
  if (a.platform() == Platform::MPS && native::mps::supports_argmax_argmin_dim(a.scalar_type())) {
    if (a.dim() == 0) {
      return Series::full({}, 0.0, ScalarType::Int64).to(a.platform());
    }
    auto aa = a.contiguous();
    const auto nd = aa.dim();
    if (dim < 0) dim += nd;
    auto out_sizes = out_sizes_for_reduce_(aa.sizes(), dim, false);
    Series out = Series::empty(out_sizes, ScalarType::Int64, aa.platform());
    native::mps::argmin_dim(aa, dim, out);
    return maybe_keepdim_(out, aa.sizes(), dim, keepdim);
  }
  require_backend_dispatch_supported_(a.platform(), "argmin");
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
      return out;
    }
    require_backend_dispatch_supported_(a.platform(), "where");
  }
  native::cpu::where(cc, aa, bb, out);
  return out;
}

Series clamp(const Series& a, double min, double max) {
  Series out = a.clone();
  clamp_(out, min, max);
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
  throw_error("clamp_: non-contiguous target not supported yet");
}

Series relu(const Series& a) {
  PLUSS_CHECK(a.defined(), "relu: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "relu: complex dtype not supported");
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Relu, a.scalar_type()),
                "relu: MPS path does not yet support this dtype");
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    native::mps::unary_math(native::mps::UnaryMathOp::Relu, a.contiguous(), out);
    return out;
  }
  Series out = a.clone();
  relu_(out);
  return out;
}

Series& relu_(Series& a) {
  PLUSS_CHECK(a.defined(), "relu_: undefined");
  PLUSS_CHECK(a.scalar_type() != ScalarType::Complex64 && a.scalar_type() != ScalarType::Complex128,
              "relu_: complex dtype not supported");
  if (a.platform() == Platform::MPS) {
    a = relu(a);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "relu_");
  auto aa = a.contiguous();
  native::cpu::relu_inplace(aa);
  if (a.is_contiguous()) {
    std::memcpy(a.raw_mutable_data(), aa.raw_data(), (std::size_t)(aa.dtype().itemsize * aa.numel()));
    return a;
  }

  throw_error("relu_: non-contiguous target not supported yet");
}

Series sigmoid(const Series& a) {
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_unary_math(native::mps::UnaryMathOp::Sigmoid, a.scalar_type()),
                "sigmoid: MPS path does not yet support this dtype");
    Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
    native::mps::unary_math(native::mps::UnaryMathOp::Sigmoid, a.contiguous(), out);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "sigmoid");
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::sigmoid(aa, out);
  return out;
}
Series& sigmoid_(Series& a) {
  if (a.platform() == Platform::MPS) {
    a = sigmoid(a);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "sigmoid_");
  PLUSS_CHECK(a.is_contiguous(), "sigmoid_: requires contiguous");
  native::cpu::sigmoid_inplace(a);
  return a;
}

Series softplus(const Series& a, double beta, double threshold) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const Series aa = a.contiguous();
    const Series beta_s = scalar_like_(aa, beta);
    const Series thresh_s = scalar_like_(aa, threshold);
    const Series bz = mul(aa, beta_s);
    const Series use_linear = gt(bz, thresh_s);
    const Series smooth = div(log1p(exp(bz)), beta_s);
    return where(use_linear, aa, smooth);
  }
  require_backend_dispatch_supported_(a.platform(), "softplus");
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::softplus(aa, out, beta, threshold);
  return out;
}
Series& softplus_(Series& a, double beta, double threshold) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    a = softplus(a, beta, threshold);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "softplus_");
  PLUSS_CHECK(a.is_contiguous(), "softplus_: requires contiguous");
  native::cpu::softplus_inplace(a, beta, threshold);
  return a;
}

Series silu(const Series& a) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const Series aa = a.contiguous();
    return mul(aa, sigmoid(aa));
  }
  require_backend_dispatch_supported_(a.platform(), "silu");
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::silu(aa, out);
  return out;
}
Series& silu_(Series& a) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    a = silu(a);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "silu_");
  PLUSS_CHECK(a.is_contiguous(), "silu_: requires contiguous");
  native::cpu::silu_inplace(a);
  return a;
}

Series gelu(const Series& a, bool approximate) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32 && approximate) {
    const Series aa = a.contiguous();
    const Series half = scalar_like_(aa, 0.5);
    const Series one = scalar_like_(aa, 1.0);
    const Series c0 = scalar_like_(aa, 0.7978845608028654); // sqrt(2/pi)
    const Series c1 = scalar_like_(aa, 0.044715);
    const Series x2 = mul(aa, aa);
    const Series x3 = mul(x2, aa);
    const Series inner = mul(c0, add(aa, mul(c1, x3)));
    return mul(mul(half, aa), add(one, tanh(inner)));
  }
  require_backend_dispatch_supported_(a.platform(), "gelu");
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::gelu(aa, out, approximate);
  return out;
}
Series& gelu_(Series& a, bool approximate) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32 && approximate) {
    a = gelu(a, approximate);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "gelu_");
  PLUSS_CHECK(a.is_contiguous(), "gelu_: requires contiguous");
  native::cpu::gelu_inplace(a, approximate);
  return a;
}

Series leaky_relu(const Series& a, double negative_slope) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const Series aa = a.contiguous();
    const Series zero = scalar_like_(aa, 0.0);
    const Series slope = scalar_like_(aa, negative_slope);
    return where(ge(aa, zero), aa, mul(aa, slope));
  }
  require_backend_dispatch_supported_(a.platform(), "leaky_relu");
  Series out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::leaky_relu(aa, out, negative_slope);
  return out;
}
Series& leaky_relu_(Series& a, double negative_slope) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    a = leaky_relu(a, negative_slope);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "leaky_relu_");
  PLUSS_CHECK(a.is_contiguous(), "leaky_relu_: requires contiguous");
  native::cpu::leaky_relu_inplace(a, negative_slope);
  return a;
}

Series softmax(const Series& a, std::int64_t dim) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const auto nd = a.dim();
    if (dim < 0) dim += nd;
    const Series aa = a.contiguous();
    const Series m = amax(aa, dim, true);
    const Series z = sub(aa, m);
    const Series e = exp(z);
    const Series s = sum(e, dim, true);
    return div(e, s);
  }
  require_backend_dispatch_supported_(a.platform(), "softmax");
  return native::cpu::softmax(a, dim);
}
Series log_softmax(const Series& a, std::int64_t dim) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const auto nd = a.dim();
    if (dim < 0) dim += nd;
    const Series aa = a.contiguous();
    const Series m = amax(aa, dim, true);
    const Series z = sub(aa, m);
    const Series lse = log(sum(exp(z), dim, true));
    return sub(z, lse);
  }
  require_backend_dispatch_supported_(a.platform(), "log_softmax");
  return native::cpu::log_softmax(a, dim);
}
Series logsumexp(const Series& a, std::int64_t dim, bool keepdim) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    const auto nd = a.dim();
    if (dim < 0) dim += nd;
    const Series aa = a.contiguous();
    const Series m = amax(aa, dim, true);
    Series out = add(log(sum(exp(sub(aa, m)), dim, true)), m);
    if (!keepdim) out = out.squeeze(dim);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "logsumexp");
  return native::cpu::logsumexp(a, dim, keepdim);
}

Series scatter(const Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter");
  require_same_platform(a, src, "scatter");
  if (a.platform() == Platform::MPS) {
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    if (a.scalar_type() == ScalarType::Float32 && src.scalar_type() == ScalarType::Float32 &&
        native::mps::supports_scatter(a.scalar_type(), idx.scalar_type(), src.scalar_type())) {
      Series out = a.contiguous();
      native::mps::scatter_set_(out, dim, idx, src.contiguous());
      return out;
    }
    PLUSS_CHECK(native::mps::supports_scatter(ScalarType::Float32, idx.scalar_type(), ScalarType::Float32),
                "scatter: no native MPS kernel yet for this dtype/op.");
    Series out_f = a.to(ScalarType::Float32).contiguous();
    Series src_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
    native::mps::scatter_set_(out_f, dim, idx, src_f);
    if (a.scalar_type() == ScalarType::Float32) return out_f;
    return out_f.to(a.scalar_type());
  }
  require_backend_dispatch_supported_(a.platform(), "scatter");
  return native::cpu::scatter(a, dim, index, src);
}
Series& scatter_(Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_");
  require_same_platform(a, src, "scatter_");
  if (a.platform() == Platform::MPS) {
    a = scatter(a, dim, index, src);
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
  throw_error("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
}

static double scatter_reduce_identity_(const std::string& reduce) {
  if (reduce == "sum") return 0.0;
  if (reduce == "amax") return -std::numeric_limits<double>::infinity();
  if (reduce == "amin") return +std::numeric_limits<double>::infinity();
  throw_error("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
}

Series scatter_add(const Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_add");
  require_same_platform(a, src, "scatter_add");
  const auto out_dt = promote_types(a.scalar_type(), src.scalar_type());
  if (a.platform() == Platform::MPS) {
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    if (out_dt == ScalarType::Float32 &&
        native::mps::supports_scatter_reduce(out_dt, idx.scalar_type(), out_dt)) {
      Series out = a.to(out_dt).contiguous();
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out, dim, idx, ss, native::mps::ScatterReduceOp::Sum);
      return out;
    }
    PLUSS_CHECK(native::mps::supports_scatter_reduce(ScalarType::Float32, idx.scalar_type(), ScalarType::Float32),
                "scatter_add: no native MPS kernel yet for this dtype/op.");
    Series out_f = a.to(ScalarType::Float32).contiguous();
    const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
    native::mps::scatter_reduce_(out_f, dim, idx, ss_f, native::mps::ScatterReduceOp::Sum);
    return out_f.to(out_dt);
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_add");
  return native::cpu::scatter_add(a, dim, index, src);
}
Series& scatter_add_(Series& a, std::int64_t dim, const Series& index, const Series& src) {
  require_same_platform(a, index, "scatter_add_");
  require_same_platform(a, src, "scatter_add_");
  if (a.platform() == Platform::MPS) {
    a = scatter_add(a, dim, index, src).to(a.scalar_type());
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
  if (a.platform() == Platform::MPS) {
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    const auto rop = mps_scatter_reduce_op_(reduce);
    if (out_dt == ScalarType::Float32 &&
        native::mps::supports_scatter_reduce(out_dt, idx.scalar_type(), out_dt)) {
      Series out = include_self
                     ? a.to(out_dt).contiguous()
                     : Series::full(a.sizes(), scatter_reduce_identity_(reduce), out_dt).to(a.platform()).contiguous();
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out, dim, idx, ss, rop);
      return out;
    }
    PLUSS_CHECK(native::mps::supports_scatter_reduce(ScalarType::Float32, idx.scalar_type(), ScalarType::Float32),
                "scatter_reduce: no native MPS kernel yet for this dtype/op.");
    Series out_f = include_self
                     ? a.to(ScalarType::Float32).contiguous()
                     : Series::full(a.sizes(), scatter_reduce_identity_(reduce), ScalarType::Float32).to(a.platform()).contiguous();
    const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
    native::mps::scatter_reduce_(out_f, dim, idx, ss_f, rop);
    return out_f.to(out_dt);
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_reduce");
  return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self);
}
Series scatter_reduce(const Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self, bool has_initial, double initial) {
  require_same_platform(a, index, "scatter_reduce");
  require_same_platform(a, src, "scatter_reduce");
  const auto out_dt = promote_types(a.scalar_type(), src.scalar_type());
  if (a.platform() == Platform::MPS) {
    const Series idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
    const auto rop = mps_scatter_reduce_op_(reduce);
    if (out_dt == ScalarType::Float32 &&
        native::mps::supports_scatter_reduce(out_dt, idx.scalar_type(), out_dt)) {
      Series out = has_initial
                     ? Series::full(a.sizes(), initial, out_dt).to(a.platform()).contiguous()
                     : (include_self
                          ? a.to(out_dt).contiguous()
                          : Series::full(a.sizes(), scatter_reduce_identity_(reduce), out_dt).to(a.platform()).contiguous());
      if (has_initial && include_self) {
        const Series aa = a.to(out_dt);
        if (reduce == "sum") out = add(out, aa);
        else if (reduce == "amax") out = maximum(out, aa);
        else if (reduce == "amin") out = minimum(out, aa);
        else throw_error("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
      }
      const Series ss = src.expand(idx.sizes(), true).to(out_dt).contiguous();
      native::mps::scatter_reduce_(out, dim, idx, ss, rop);
      return out;
    }
    PLUSS_CHECK(native::mps::supports_scatter_reduce(ScalarType::Float32, idx.scalar_type(), ScalarType::Float32),
                "scatter_reduce: no native MPS kernel yet for this dtype/op.");
    Series out_f = has_initial
                     ? Series::full(a.sizes(), initial, ScalarType::Float32).to(a.platform()).contiguous()
                     : (include_self
                          ? a.to(ScalarType::Float32).contiguous()
                          : Series::full(a.sizes(), scatter_reduce_identity_(reduce), ScalarType::Float32).to(a.platform()).contiguous());
    if (has_initial && include_self) {
      const Series aa_f = a.to(ScalarType::Float32);
      if (reduce == "sum") out_f = add(out_f, aa_f);
      else if (reduce == "amax") out_f = maximum(out_f, aa_f);
      else if (reduce == "amin") out_f = minimum(out_f, aa_f);
      else throw_error("scatter_reduce: unsupported reduce (expected sum|amax|amin)");
    }
    const Series ss_f = src.expand(idx.sizes(), true).to(ScalarType::Float32).contiguous();
    native::mps::scatter_reduce_(out_f, dim, idx, ss_f, rop);
    return out_f.to(out_dt);
  }
  require_backend_dispatch_supported_(a.platform(), "scatter_reduce");
  return native::cpu::scatter_reduce(a, dim, index, src, reduce, include_self, has_initial, initial);
}
Series& scatter_reduce_(Series& a, std::int64_t dim, const Series& index, const Series& src, const std::string& reduce, bool include_self) {
  require_same_platform(a, index, "scatter_reduce_");
  require_same_platform(a, src, "scatter_reduce_");
  if (a.platform() == Platform::MPS) {
    a = scatter_reduce(a, dim, index, src, reduce, include_self).to(a.scalar_type());
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
  require_backend_dispatch_supported_(a.platform(), "scatter_reduce_");
  native::cpu::scatter_reduce_inplace(a, dim, index, src, reduce, include_self, has_initial, initial);
  return a;
}

Series gather(const Series& input, std::int64_t dim, const Series& index) {
  require_same_platform(input, index, "gather");
  auto in = input.contiguous();
  auto idx = (index.scalar_type() == ScalarType::Int64 ? index : index.to(ScalarType::Int64)).contiguous();
  if (input.platform() == Platform::MPS) {
    if (native::mps::supports_gather(in.scalar_type(), idx.scalar_type())) {
      Series out = Series::empty(std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end()),
                                 in.scalar_type(),
                                 in.platform());
      native::mps::gather(in, dim, idx, out);
      return out;
    }
    PLUSS_CHECK(native::mps::supports_gather(ScalarType::Float32, idx.scalar_type()),
                "gather: no native MPS kernel yet for this dtype/op.");
    Series in_f = in.to(ScalarType::Float32);
    Series out_f = Series::empty(std::vector<std::int64_t>(idx.sizes().begin(), idx.sizes().end()),
                                 ScalarType::Float32,
                                 in.platform());
    native::mps::gather(in_f, dim, idx, out_f);
    return out_f.to(input.scalar_type());
  }
  require_backend_dispatch_supported_(input.platform(), "gather");
  return native::cpu::gather(in, dim, idx);
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
  require_backend_dispatch_supported_(a.platform(), "atan2");
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
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
  require_backend_dispatch_supported_(a.platform(), "fmod");
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
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
  require_backend_dispatch_supported_(a.platform(), "remainder");
  auto out = Series::empty(SeriesIterator::broadcast_shape(a.sizes(), b.sizes()), promote_types(a.scalar_type(), b.scalar_type()), a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  auto bb = b.contiguous().to(out.scalar_type());
  native::cpu::remainder(aa, bb, out);
  return out;
}

Series rsqrt(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    return reciprocal(sqrt(aa));
  }
  require_backend_dispatch_supported_(a.platform(), "rsqrt");
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::rsqrt(aa, out);
  return out;
}
Series log2(const Series& a) {
  if (a.platform() == Platform::MPS) {
    auto aa = a.contiguous().to(ScalarType::Float32);
    const Series ln2 = scalar_like_(aa, 0.69314718055994530942);
    return div(log(aa), ln2);
  }
  require_backend_dispatch_supported_(a.platform(), "log2");
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
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
  require_backend_dispatch_supported_(a.platform(), "log10");
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
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
  require_backend_dispatch_supported_(a.platform(), "exp2");
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
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
  require_backend_dispatch_supported_(a.platform(), "cbrt");
  auto out = Series::empty(a.sizes(), promote_types(a.scalar_type(), ScalarType::Float32), a.platform());
  auto aa = a.contiguous().to(out.scalar_type());
  native::cpu::cbrt(aa, out);
  return out;
}
Series square(const Series& a) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    auto aa = a.contiguous();
    return mul(aa, aa);
  }
  require_backend_dispatch_supported_(a.platform(), "square");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::square(aa, out);
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
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    return maximum(a, scalar_like_(a, lo));
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_min");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::clamp_min(aa, lo, out);
  return out;
}
Series& clamp_min_(Series& a, double lo) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
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
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    return minimum(a, scalar_like_(a, hi));
  }
  require_backend_dispatch_supported_(a.platform(), "clamp_max");
  auto out = Series::empty(a.sizes(), a.scalar_type(), a.platform());
  auto aa = a.contiguous();
  native::cpu::clamp_max(aa, hi, out);
  return out;
}
Series& clamp_max_(Series& a, double hi) {
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
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
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    Series out = a.clone();
    masked_scatter_(out, mask, src);
    return out;
  }
  require_backend_dispatch_supported_(a.platform(), "masked_scatter");
  Series out = a.clone();
  masked_scatter_(out, mask, src);
  return out;
}

Series& masked_scatter_(Series& a, const Series& mask, const Series& src) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "masked_scatter_: mask must be bool");
  require_same_platform(a, mask, "masked_scatter_");
  require_same_platform(a, src, "masked_scatter_");
  if (a.platform() == Platform::MPS && a.scalar_type() == ScalarType::Float32) {
    PLUSS_CHECK(a.is_contiguous(), "masked_scatter_: requires contiguous Series on MPS");
    auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
    PLUSS_CHECK(std::equal(a.sizes().begin(), a.sizes().end(), shape.begin(), shape.end()),
                "masked_scatter_: shape mismatch (require a already broadcasted)");
    const std::array<std::int64_t, 1> flat_shape{{a.numel()}};
    const Series mm_flat = mask.expand(shape, true).contiguous().reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    const Series nz2d = nonzero(mm_flat);
    const std::int64_t count = nz2d.numel();
    PLUSS_CHECK(src.numel() == count, "masked_scatter_: src numel must match number of True in mask");
    if (count == 0) {
      return a;
    }
    const Series idx = nz2d.squeeze(-1).contiguous();
    const std::array<std::int64_t, 1> src_shape{{count}};
    const Series ss = src.contiguous().to(a.scalar_type()).reshape(std::span<const std::int64_t>(src_shape.data(), src_shape.size()));
    Series a_flat = a.reshape(std::span<const std::int64_t>(flat_shape.data(), flat_shape.size()));
    a_flat = scatter(a_flat, 0, idx, ss);
    a = a_flat.reshape(shape);
    return a;
  }
  require_backend_dispatch_supported_(a.platform(), "masked_scatter_");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
  PLUSS_CHECK(std::equal(a.sizes().begin(), a.sizes().end(), shape.begin(), shape.end()),
              "masked_scatter_: shape mismatch (require a already broadcasted)");
  auto mm = mask.expand(shape, true).contiguous();
  const bool* mp = reinterpret_cast<const bool*>(mm.raw_data());
  const auto n = a.numel();
  std::int64_t count = 0;
  for (std::int64_t i=0;i<n;++i) if (mp[i]) ++count;
  PLUSS_CHECK(src.numel() == count, "masked_scatter_: src numel must match number of True in mask");
  auto ss = src.contiguous().to(a.scalar_type());

  const auto item = a.dtype().itemsize;
  std::byte* ap = reinterpret_cast<std::byte*>(a.raw_mutable_data());
  const std::byte* sp = ss.raw_data();
  std::int64_t j = 0;
  for (std::int64_t i=0;i<n;++i) {
    if (!mp[i]) continue;
    std::memcpy(ap + i*item, sp + j*item, (std::size_t)item);
    ++j;
  }
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
    default: throw_error("read_i64_element: expected integral/bool dtype");
  }
}

Series unique(const Series& a, bool sorted) {
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(a.dim() == 1 || a.numel() == 0, "unique: MPS path currently supports 1D tensors only");
    PLUSS_CHECK(sorted, "unique: MPS path currently requires sorted=true");
    auto x = a.contiguous();
    PLUSS_CHECK(x.scalar_type() == ScalarType::Int64,
                "unique: MPS path currently supports int64 tensors only");
    auto counts = bincount(x, 0);
    if (counts.numel() == 0) {
      return Series::empty({0}, ScalarType::Int64, Platform::MPS);
    }
    auto present = counts.to(ScalarType::Float32).gt(0.0);
    auto nz = nonzero(present);
    if (nz.numel() == 0) {
      return Series::empty({0}, ScalarType::Int64, Platform::MPS);
    }
    return nz.squeeze(-1).to(ScalarType::Int64);
  }

  PLUSS_CHECK(a.dim() == 1 || a.numel() == 0, "unique: only 1D supported in this CPU reference implementation");
  auto x = a.contiguous();
  // For float NaN: treat NaNs as equal for uniqueness, similar to Torch's default behavior in CPU path.
  // We'll normalize by mapping NaNs to a single sentinel during comparison but keep original values.
  if (x.scalar_type() == ScalarType::Float32) {
    const auto* p = reinterpret_cast<const float*>(x.raw_data());
    std::vector<float> v(p, p + x.numel());
    if (sorted) std::sort(v.begin(), v.end(), [](float u,float w){
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
    const std::array<std::int64_t, 1> os{{(std::int64_t)v.size()}};
    Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Float32, x.platform());
    std::memcpy(out.raw_mutable_data(), v.data(), v.size()*sizeof(float));
    return out;
  }
  if (x.scalar_type() == ScalarType::Float64) {
    const auto* p = reinterpret_cast<const double*>(x.raw_data());
    std::vector<double> v(p, p + x.numel());
    if (sorted) std::sort(v.begin(), v.end(), [](double u,double w){
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
    const std::array<std::int64_t, 1> os{{(std::int64_t)v.size()}};
    Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Float64, x.platform());
    std::memcpy(out.raw_mutable_data(), v.data(), v.size()*sizeof(double));
    return out;
  }
  // Integer/bool path
  auto st = x.scalar_type();
  PLUSS_CHECK(dtype_of(st).is_integral() || st == ScalarType::Bool, "unique: dtype not supported");
  std::vector<std::int64_t> vv;
  vv.reserve((std::size_t)x.numel());
  for (std::int64_t i=0;i<x.numel();++i) vv.push_back(read_i64_element(x, i));
  if (sorted) std::sort(vv.begin(), vv.end());
  vv.erase(std::unique(vv.begin(), vv.end()), vv.end());
  const std::array<std::int64_t, 1> os{{(std::int64_t)vv.size()}};
  Series out = Series::empty(std::span<const std::int64_t>(os.data(), os.size()), ScalarType::Int64, x.platform());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_mutable_data());
  for (std::size_t i=0;i<vv.size();++i) op[i]=vv[i];
  return out;
}

Series bincount(const Series& a, std::int64_t minlength) {
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(a.scalar_type() == ScalarType::Int64,
                "bincount: MPS path currently supports int64 input only");
    auto x = a.contiguous();
    if (x.numel() == 0) {
      const auto outn = std::max<std::int64_t>(0, minlength);
      return Series::zeros({outn}, ScalarType::Int64).to(Platform::MPS);
    }

    auto xf = x.to(ScalarType::Float32);
    auto min_s = amin(xf);
    auto max_s = amax(xf);
    const auto min_h = min_s.to_host<float>();
    const auto max_h = max_s.to_host<float>();
    PLUSS_CHECK(!min_h.data.empty() && !max_h.data.empty(), "bincount: internal scalar extraction failure");
    PLUSS_CHECK(min_h.data[0] >= 0.0f, "bincount: input must be non-negative");
    const auto maxv = static_cast<std::int64_t>(std::llround(static_cast<double>(max_h.data[0])));
    const auto outn = std::max<std::int64_t>(minlength, maxv + 1);

    auto src = Series::ones({x.numel()}, ScalarType::Float32).to(Platform::MPS);
    auto out = Series::zeros({outn}, ScalarType::Float32).to(Platform::MPS);
    out = scatter_add(out, 0, x, src);
    return out.to(ScalarType::Int64);
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
  if (a.platform() == Platform::MPS) {
    auto x = a.contiguous().to(ScalarType::Float32);
    const auto n = x.numel();

    if (min == 0.0 && max == 0.0) {
      auto mn_s = amin(x);
      auto mx_s = amax(x);
      const auto mn_h = mn_s.to_host<float>();
      const auto mx_h = mx_s.to_host<float>();
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
  if (a.platform() == Platform::MPS) {
    // Correctness-first route for MPS until dedicated nonzero kernel lands.
    const Series cpu = a.to(Platform::CPU);
    return nonzero(cpu).to(Platform::MPS);
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
  PLUSS_CHECK(
      native::mps::supports_scatter(ScalarType::Float32, ScalarType::Int64, ScalarType::Float32),
      std::string(op_name) + ": native MPS scatter kernel unavailable");
  const auto nd = xs_in.front().dim();
  PLUSS_CHECK(nd >= 1, std::string(op_name) + ": scalar tensors are not supported");

  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(op_name) + ": dim out of range");

  std::vector<Series> xs;
  xs.reserve(xs_in.size());
  for (const auto& t : xs_in) {
    PLUSS_CHECK(t.defined(), std::string(op_name) + ": undefined tensor");
    PLUSS_CHECK(t.platform() == Platform::MPS, std::string(op_name) + ": expected MPS tensors");
    PLUSS_CHECK(t.scalar_type() == ScalarType::Float32,
                std::string(op_name) + ": current MPS path supports float32 tensors only");
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

  Series out = Series::empty(out_sizes, ScalarType::Float32, Platform::MPS);
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
    native::mps::scatter_set_(out, dim, idx, t);
    offset += dlen;
  }
  return out;
}

} // namespace pluss::codem

namespace pluss::codem {

using namespace ops_internal;

Series Series::undefined() {
  return Series();
}

Series Series::empty(std::span<const std::int64_t> sizes, ScalarType st) {
  return empty(sizes, st, scoped_platform_or_cpu_());
}

Series Series::empty(std::span<const std::int64_t> sizes, ScalarType st, Platform platform) {
  if (platform == Platform::CUDA) {
    throw_error("Series::empty(..., CUDA): CUDA backend not built yet");
  }
  if (platform == Platform::MPS) {
    if (!native::mps::runtime_available()) {
      throw_error(
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


Series Series::make_from_doubles(const std::vector<double>& flat, std::span<const std::int64_t> sizes, ScalarType st) {
  const std::int64_t n = compute_numel(sizes);
  PLUSS_CHECK((std::int64_t)flat.size() == n, "make_from_doubles: size mismatch");
  Series out = Series::empty(sizes, st);
  // Write via float64 temp for simplicity, then cast if needed.
  Series tmp = Series::empty(sizes, ScalarType::Float64);
  std::memcpy(tmp.raw_mutable_data(), flat.data(), sizeof(double) * (std::size_t)n);
  out.copy_(tmp.to(st), false);
  return out;
}

Series Series::zeros(std::span<const std::int64_t> sizes, ScalarType st) {
  auto t = empty(sizes, st);
  const auto dt = t.dtype();
  std::memset(t.raw_data(), 0, static_cast<std::size_t>(t.numel()) * dt.itemsize);
  return t;
}

Series Series::ones(std::span<const std::int64_t> sizes, ScalarType st) {
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
  throw_error("ones: dtype not implemented");
}

Series Series::full(std::span<const std::int64_t> sizes, double value, ScalarType st) {
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
  throw_error("full: dtype not implemented");
}

Series Series::view(std::span<const std::int64_t> new_sizes) const {
  if (!defined()) throw_error("view: undefined");
  if (!can_view_as(*this, new_sizes)) throw_error("view: requires contiguous in v0");
  auto impl = pluss::msharp::make_intrusive<SeriesImpl>(
      impl_->storage(), impl_->storage_offset_elems(),
      std::vector<std::int64_t>(new_sizes.begin(), new_sizes.end()),
      contiguous_strides(new_sizes),
      scalar_type(),
      platform());
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

Series Series::reshape(std::span<const std::int64_t> new_sizes) const {
  if (!defined()) throw_error("reshape: undefined");
  if (can_view_as(*this, new_sizes)) return view(new_sizes);
  return contiguous().view(new_sizes);
}

Series Series::transpose(std::int64_t d0, std::int64_t d1) const {
  if (!defined()) throw_error("transpose: undefined");
  const auto nd = dim();
  if (d0 < 0) d0 += nd;
  if (d1 < 0) d1 += nd;
  if (d0 < 0 || d0 >= nd || d1 < 0 || d1 >= nd) throw_error("transpose: dim out of range");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}


Series Series::permute(std::span<const std::int64_t> dims) const {
  if (!defined()) throw_error("permute: undefined");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

Series Series::unsqueeze(std::int64_t dim_) const {
  if (!defined()) throw_error("unsqueeze: undefined");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

Series Series::squeeze() const {
  if (!defined()) throw_error("squeeze: undefined");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

Series Series::squeeze(std::int64_t dim_) const {
  if (!defined()) throw_error("squeeze(dim): undefined");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

Series Series::flatten(std::int64_t start_dim, std::int64_t end_dim) const {
  if (!defined()) throw_error("flatten: undefined");
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
  if (!defined()) throw_error("norm: undefined");
  // Complex not supported for now (Torch supports abs-based norm; we can add later)
  // complex norms use abs-based definition

  if (p == 2.0) {
    auto y = ops_internal::sum(this->mul(*this));
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
  if (!defined()) throw_error("norm(dim): undefined");
  // complex norms use abs-based definition

  if (p == 2.0) {
    auto y = this->mul(*this).sum(dim_, keepdim);
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
  if (!defined()) throw_error("clone: undefined");

  Series out = Series::empty(sizes(), scalar_type(), platform());
  SeriesIterator it = SeriesIterator::unary(*this, out);
  it.copy_out_from_in(*this, out);
  return out;
}

Series Series::contiguous() const {
  if (!defined()) throw_error("contiguous: undefined");
  if (is_contiguous()) return *this;

  Series out = Series::empty(sizes(), scalar_type(), platform());
  SeriesIterator it = SeriesIterator::unary(*this, out);
  it.copy_out_from_in(*this, out);
  return out;
}

Series Series::platform(Platform target) const {
  if (!defined()) throw_error("platform: undefined");
  const auto src_platform = platform();
  if (src_platform == target) {
    return *this;
  }

  switch (target) {
    case Platform::CPU:
      break;
    case Platform::CUDA:
      throw_error("Series::platform(CUDA): CUDA backend not built yet");
    case Platform::MPS:
      if (!native::mps::runtime_available()) {
        throw_error(
            std::string("Series::platform(MPS): MPS runtime unavailable: ") +
            native::mps::runtime_error_cstr());
      }
      PLUSS_CHECK(
          scalar_type() != ScalarType::Float64,
          "Series::platform(MPS): float64 is unsupported on MPS; cast to float32/float16/bfloat16 first");
      break;
    default:
      throw_error("Series::platform: unsupported platform");
  }

  auto src = contiguous();
  Series out = Series::empty(src.sizes(), src.scalar_type(), target);
  const auto nbytes =
      static_cast<std::size_t>(src.numel()) * static_cast<std::size_t>(src.itemsize());
  if (nbytes > 0) {
    std::memcpy(out.raw_mutable_data(), src.raw_data(), nbytes);
  }
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
  if (!defined()) throw_error("to: undefined");
  if (dst == scalar_type()) return *this;
  if (platform() == Platform::MPS) {
    PLUSS_CHECK(dst != ScalarType::Float64,
                "Series::to(float64): float64 is unsupported on MPS");
  }

  auto src = contiguous();
  Series out = Series::empty(src.sizes(), dst, platform());
  const auto n = src.numel();

  if (platform() == Platform::MPS) {
    PLUSS_CHECK(native::mps::supports_cast(src.scalar_type(), dst),
                "Series::to(dtype): no native MPS cast kernel for this source/destination dtype");
    native::mps::cast(src, out);
    return out;
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
      case ScalarType::Float32: return reinterpret_cast<const float*>(src.raw_data())[i];
      case ScalarType::Float64: return reinterpret_cast<const double*>(src.raw_data())[i];
      default: throw_error("to: unsupported source dtype"); return 0.0;
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
      case ScalarType::Float32: reinterpret_cast<float*>(out.raw_data())[i] = static_cast<float>(v); return;
      case ScalarType::Float64: reinterpret_cast<double*>(out.raw_data())[i] = v; return;
      default: throw_error("to: unsupported destination dtype"); return;
    }
  };

  for (std::int64_t i=0; i<n; ++i) {
    const double v = load_as_f64(src.scalar_type(), i);
    store_from_f64(dst, i, v);
  }
  return out;
}

Series Series::select(std::int64_t dim, std::int64_t index) const {
  if (!defined()) throw_error("select: undefined");
  auto nd = this->dim();
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) throw_error("select: dim out of range");
  auto sizes_v = std::vector<std::int64_t>(sizes().begin(), sizes().end());
  auto strides_v = std::vector<std::int64_t>(strides().begin(), strides().end());
  const auto size_dim = sizes_v[static_cast<std::size_t>(dim)];
  if (index < 0) index += size_dim;
  if (index < 0 || index >= size_dim) throw_error("select: index out of range");
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
  return ops_internal::SeriesFactoryAccess::wrap(std::move(impl));
}

static void ensure_writable_(Series& self) {
  if (!self.defined()) throw_error("inplace: undefined");
  // detach if shared
  if (!self.SeriesBase::unique()) {
    self = self.clone();
  }
  if (!self.is_contiguous()) {
    self = self.contiguous().clone();
  }
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

Series& Series::add_(const Series& rhs, double alpha) {
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "add_");
  Series b = expand_to_self_(rhs, *this).to(scalar_type());
  if (alpha != 1.0) {
    b = b.mul(alpha).to(scalar_type());
  }
  *this = ops_internal::add(*this, b);
  return *this;
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
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "sub_");
  Series b = expand_to_self_(rhs, *this).to(scalar_type());
  if (alpha != 1.0) {
    b = b.mul(alpha).to(scalar_type());
  }
  *this = ops_internal::sub(*this, b);
  return *this;
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
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "mul_");
  auto b = expand_to_self_(rhs, *this).to(scalar_type());
  *this = ops_internal::mul(*this, b);
  return *this;
}

Series& Series::div_(const Series& rhs) {
  ensure_writable_(*this);
  require_same_platform(*this, rhs, "div_");
  auto b = expand_to_self_(rhs, *this).to(scalar_type());
  *this = ops_internal::div(*this, b);
  return *this;
}

Series Series::arange(double start, double end, double step, ScalarType dtype) {
  if (step == 0.0) throw_error("arange: step cannot be 0");
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
      default: throw_error("arange: dtype not implemented");
    }
  }
  return out;
}

Series Series::linspace(double start, double end, std::int64_t steps, ScalarType dtype) {
  if (steps < 0) throw_error("linspace: steps must be >= 0");
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
      default: throw_error("linspace: dtype not implemented (float types only for now)");
    }
  }
  return out;
}

Series Series::eye(std::int64_t n, std::int64_t m, ScalarType dtype) {
  if (n < 0 || m < 0) throw_error("eye: sizes must be >= 0");
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
      default: throw_error("eye: dtype not implemented");
    }
  }
  return out;
}

Series Series::rand(std::span<const std::int64_t> sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    throw_error("rand: dtype must be float");
  Series out = Series::empty(sizes, dtype);
  auto state = seed ? seed : 0x123456789abcdef0ULL;
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

Series Series::randn(std::span<const std::int64_t> sizes, ScalarType dtype, std::uint64_t seed) {
  if (!(dtype == ScalarType::Float32 || dtype == ScalarType::Float64 || dtype == ScalarType::Float16 || dtype == ScalarType::BFloat16))
    throw_error("randn: dtype must be float");
  Series out = Series::empty(sizes, dtype);
  auto state = seed ? seed : 0x0f1e2d3c4b5a6978ULL;
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
  if (!storage.defined()) throw_error("from_storage: undefined storage");
  // Interop / external views must be non-overlapping & dense (debug-enforced).
  check_view_invariants(sizes, strides);
  return ops_internal::SeriesFactoryAccess::wrap(
      pluss::msharp::make_intrusive<SeriesImpl>(storage, storage_offset, sizes, strides, dtype, Platform::CPU));
}

static Series bitwise_binary_(const Series& a, const Series& b,
                              void(*fn)(const Series&, const Series&, const Series&)) {
  require_same_platform(a, b, "bitwise op");
  require_backend_dispatch_supported_(a.platform(), "bitwise op");
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  Series out = Series::empty(shape, a.scalar_type(), a.platform());
  fn(a.expand(out.sizes()), b.expand(out.sizes()), out);
  return out;
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
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_and);
}
Series Series::bitwise_or(const Series& rhs) const {
  if (platform() == Platform::MPS) {
    require_same_platform(*this, rhs, "bitwise_or");
    if (scalar_type() == ScalarType::Bool && rhs.scalar_type() == ScalarType::Bool) {
      return ops_internal::logical_or(*this, rhs);
    }
    return bitwise_binary_mps_(*this, rhs, MpsBitwiseOp::Or);
  }
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_or);
}
Series Series::bitwise_xor(const Series& rhs) const {
  if (platform() == Platform::MPS) {
    require_same_platform(*this, rhs, "bitwise_xor");
    if (scalar_type() == ScalarType::Bool && rhs.scalar_type() == ScalarType::Bool) {
      return ops_internal::logical_xor(*this, rhs);
    }
    return bitwise_binary_mps_(*this, rhs, MpsBitwiseOp::Xor);
  }
  return bitwise_binary_(*this, rhs, native::cpu::bitwise_xor);
}

Series Series::bitwise_not() const {
  if (platform() == Platform::MPS) {
    return bitwise_not_mps_(*this);
  }
  require_backend_dispatch_supported_(platform(), "bitwise_not");
  Series out = Series::empty(sizes(), scalar_type(), platform());
  native::cpu::bitwise_not(*this, out);
  return out;
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

Series& Series::exp_() { *this = exp(); return *this; }
Series& Series::log_() { *this = log(); return *this; }
Series& Series::sqrt_() { *this = sqrt(); return *this; }
Series& Series::sin_() { *this = sin(); return *this; }
Series& Series::cos_() { *this = cos(); return *this; }
Series& Series::tanh_() { *this = tanh(); return *this; }

Series Series::unite(const std::vector<Series>& others, std::int64_t dim) const {
  for (const auto& t : others) {
    require_same_platform(*this, t, "unite");
  }
  if (platform() == Platform::MPS) {
    std::vector<Series> xs;
    xs.reserve(others.size() + 1);
    xs.push_back(*this);
    for (const auto& t : others) xs.push_back(t);
    return ops_internal::mps_cat_along_dim_(xs, dim, "unite");
  }
  require_backend_dispatch_supported_(platform(), "unite");
  std::vector<Series> xs;
  xs.reserve(others.size() + 1);
  xs.push_back(*this);
  for (auto& t : others) xs.push_back(t);
  return native::cpu::cat(xs, dim);
}

Series Series::pile(const std::vector<Series>& others, std::int64_t dim) const {
  for (const auto& t : others) {
    require_same_platform(*this, t, "pile");
  }
  if (platform() == Platform::MPS) {
    const auto nd = this->dim() + 1;
    if (dim < 0) dim += nd;
    PLUSS_CHECK(dim >= 0 && dim < nd, "pile: dim out of range");
    std::vector<Series> xs;
    xs.reserve(others.size() + 1);
    xs.push_back(this->unsqueeze(dim).contiguous());
    for (const auto& t : others) xs.push_back(t.unsqueeze(dim).contiguous());
    return ops_internal::mps_cat_along_dim_(xs, dim, "pile");
  }
  require_backend_dispatch_supported_(platform(), "pile");
  std::vector<Series> xs;
  xs.reserve(others.size() + 1);
  xs.push_back(*this);
  for (auto& t : others) xs.push_back(t);
  return native::cpu::stack(xs, dim);
}

Series Series::gather(std::int64_t dim, const Series& index) const {
  return ops_internal::gather(*this, dim, index);
}
Series Series::factorial(std::int64_t n) const {
  (void)n;
  throw_error("factorial: not implemented");
}
Series Series::scatter(std::int64_t dim, const Series& index, const Series& src) const {
  return ops_internal::scatter(*this, dim, index, src);
}

Series& Series::scatter_(std::int64_t dim, const Series& index, const Series& src) {
  return ops_internal::scatter_(*this, dim, index, src);
}

Series Series::scatter_add(std::int64_t dim, const Series& index, const Series& src) const {
  return ops_internal::scatter_add(*this, dim, index, src);
}

Series& Series::scatter_add_(std::int64_t dim, const Series& index, const Series& src) {
  return ops_internal::scatter_add_(*this, dim, index, src);
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
Series Series::pow(double rhs) const { return this->pow(scalar_to_series_(rhs, scalar_type(), platform())); }
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
Series& Series::pow_(double rhs) { Series out = this->pow(rhs); this->copy_(out); return *this; }
Series& Series::minimum_(double rhs) { Series out = this->minimum(rhs); this->copy_(out); return *this; }
Series& Series::maximum_(double rhs) { Series out = this->maximum(rhs); this->copy_(out); return *this; }

Series Series::index(const std::vector<Series>& indices) const {
  for (const auto& idx : indices) {
    require_same_platform(*this, idx, "index");
  }
  if (platform() == Platform::MPS) {
    PLUSS_CHECK(indices.size() == 1, "index: MPS path currently supports exactly one index tensor");
    return take(indices.front());
  }
  require_backend_dispatch_supported_(platform(), "index");
  return pluss::codem::native::cpu::index(*this, indices);
}
Series Series::take(const Series& index) const {
  require_same_platform(*this, index, "take");
  if (platform() == Platform::MPS) {
    // Correctness-first route for MPS until dedicated take kernel lands.
    const Series cpu_self = this->to(Platform::CPU);
    const Series cpu_idx = index.to(Platform::CPU);
    return pluss::codem::native::cpu::take(cpu_self, cpu_idx).to(Platform::MPS);
  }
  require_backend_dispatch_supported_(platform(), "take");
  return pluss::codem::native::cpu::take(*this, index);
}
Series Series::put_(const Series& index, const Series& source, bool accumulate) {
  require_same_platform(*this, index, "put_");
  require_same_platform(*this, source, "put_");
  if (platform() == Platform::MPS) {
    // Correctness-first route for MPS until dedicated put_ kernel lands.
    const Series cpu_self = this->to(Platform::CPU);
    const Series cpu_idx = index.to(Platform::CPU);
    const Series cpu_src = source.to(Platform::CPU);
    const Series cpu_out = pluss::codem::native::cpu::put(cpu_self, cpu_idx, cpu_src, accumulate);
    copy_(cpu_out.to(Platform::MPS));
    return *this;
  }
  require_backend_dispatch_supported_(platform(), "put_");
  Series out = pluss::codem::native::cpu::put(*this, index, source, accumulate);
  this->copy_(out);
  return *this;
}
Series Series::nonzero() const { return pluss::codem::ops_internal::nonzero(*this); }

Series Series::tan() const { return ops_internal::tan(*this); }
Series& Series::tan_() { *this = tan(); return *this; }

Series Series::asin() const { return ops_internal::asin(*this); }
Series& Series::asin_() { *this = asin(); return *this; }

Series Series::acos() const { return ops_internal::acos(*this); }
Series& Series::acos_() { *this = acos(); return *this; }

Series Series::atan() const { return ops_internal::atan(*this); }
Series& Series::atan_() { *this = atan(); return *this; }

Series Series::sinh() const { return ops_internal::sinh(*this); }
Series& Series::sinh_() { *this = sinh(); return *this; }

Series Series::cosh() const { return ops_internal::cosh(*this); }
Series& Series::cosh_() { *this = cosh(); return *this; }

Series Series::floor() const { return ops_internal::floor(*this); }
Series& Series::floor_() { *this = floor(); return *this; }

Series Series::ceil() const { return ops_internal::ceil(*this); }
Series& Series::ceil_() { *this = ceil(); return *this; }

Series Series::round() const { return ops_internal::round(*this); }
Series& Series::round_() { *this = round(); return *this; }

Series Series::trunc() const { return ops_internal::trunc(*this); }
Series& Series::trunc_() { *this = trunc(); return *this; }

Series Series::reciprocal() const { return ops_internal::reciprocal(*this); }
Series& Series::reciprocal_() { *this = reciprocal(); return *this; }

Series Series::sigmoid() const { return ops_internal::sigmoid(*this); }
Series& Series::sigmoid_() { *this = sigmoid(); return *this; }

Series Series::relu() const { return ops_internal::relu(*this); }
Series& Series::relu_() { *this = relu(); return *this; }

Series& Series::logical_not_() { *this = ops_internal::logical_not(*this); return *this; }

Series Series::masked_select(const Series& mask) const {return pluss::codem::ops_internal::masked_select(*this, mask);}

Series Series::masked_scatter(const Series& mask, const Series& src) const {return pluss::codem::ops_internal::masked_scatter(*this, mask, src);}

Series& Series::masked_scatter_(const Series& mask, const Series& src) {return pluss::codem::ops_internal::masked_scatter_(*this, mask, src);}

Series Series::unique(bool sorted) const {return pluss::codem::ops_internal::unique(*this, sorted);}

Series Series::bincount(std::int64_t minlength) const {return pluss::codem::ops_internal::bincount(*this, minlength);}

Series Series::histc(std::int64_t bins, double min, double max) const {return pluss::codem::ops_internal::histc(*this, bins, min, max);}

Series Series::log1p() const {
  PLUSS_CALL_GUARD();
  return ops_internal::log1p(*this);
}
Series& Series::log1p_() {
  PLUSS_CALL_GUARD();
  *this = log1p();
  return *this;
}

Series Series::expm1() const {
  PLUSS_CALL_GUARD();
  return ops_internal::expm1(*this);
}
Series& Series::expm1_() {
  PLUSS_CALL_GUARD();
  *this = expm1();
  return *this;
}

Series Series::erf() const {
  PLUSS_CALL_GUARD();
  return ops_internal::erf(*this);
}
Series& Series::erf_() {
  PLUSS_CALL_GUARD();
  *this = erf();
  return *this;
}

Series Series::erfc() const {
  PLUSS_CALL_GUARD();
  return ops_internal::erfc(*this);
}
Series& Series::erfc_() {
  PLUSS_CALL_GUARD();
  *this = erfc();
  return *this;
}

Series Series::lgamma() const {
  PLUSS_CALL_GUARD();
  return ops_internal::lgamma(*this);
}
Series& Series::lgamma_() {
  PLUSS_CALL_GUARD();
  *this = lgamma();
  return *this;
}

Series Series::asinh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::asinh(*this);
}
Series& Series::asinh_() {
  PLUSS_CALL_GUARD();
  *this = asinh();
  return *this;
}

Series Series::acosh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::acosh(*this);
}
Series& Series::acosh_() {
  PLUSS_CALL_GUARD();
  *this = acosh();
  return *this;
}

Series Series::atanh() const {
  PLUSS_CALL_GUARD();
  return ops_internal::atanh(*this);
}
Series& Series::atanh_() {
  PLUSS_CALL_GUARD();
  *this = atanh();
  return *this;
}

Series Series::isnan() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isnan(*this);
}
Series& Series::isnan_() {
  PLUSS_CALL_GUARD();
  Series out = isnan().to(scalar_type());
  copy_(out);
  return *this;
}

Series Series::isinf() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isinf(*this);
}
Series& Series::isinf_() {
  PLUSS_CALL_GUARD();
  Series out = isinf().to(scalar_type());
  copy_(out);
  return *this;
}

Series Series::isfinite() const {
  PLUSS_CALL_GUARD();
  return ops_internal::isfinite(*this);
}
Series& Series::isfinite_() {
  PLUSS_CALL_GUARD();
  Series out = isfinite().to(scalar_type());
  copy_(out);
  return *this;
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
  return ops_internal::where(condition, *this, other);
}

Series& Series::where_(const Series& condition, const Series& other) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = where(condition, other);
  copy_(out);
  return *this;
}

Series Series::clamp_min(double min) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::clamp_min(*this, min);
}

Series& Series::clamp_min_(double min) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_min(min);
  copy_(out);
  return *this;
}

Series Series::clamp_max(double max) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::clamp_max(*this, max);
}

Series& Series::clamp_max_(double max) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = clamp_max(max);
  copy_(out);
  return *this;
}

Series Series::nan_to_num(double nan, double posinf, double neginf) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  return ops_internal::nan_to_num(*this, nan, posinf, neginf);
}

Series& Series::nan_to_num_(double nan, double posinf, double neginf) {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  Series out = nan_to_num(nan, posinf, neginf);
  copy_(out);
  return *this;
}

Series Series::cumsum(std::int64_t dim, bool keepdim) const {
  PLUSS_CALL_GUARD();
  ops_internal::PlatformScope platform_scope{platform()};
  (void)keepdim; // same shape output; keepdim for API parity
  Series a = is_contiguous() ? *this : contiguous();
  Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
  if (platform() == Platform::MPS && native::mps::supports_cumsum_dim(a.scalar_type())) {
    native::mps::cumsum_dim(a, dim, out);
    return out;
  }
  require_backend_dispatch_supported_(platform(), "cumsum");
  native::cpu::cumsum_dim(a, dim, out);
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
  Series out = Series::empty(a.sizes(), a.scalar_type(), platform());
  if (platform() == Platform::MPS && native::mps::supports_cumprod_dim(a.scalar_type())) {
    native::mps::cumprod_dim(a, dim, out);
    return out;
  }
  require_backend_dispatch_supported_(platform(), "cumprod");
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
