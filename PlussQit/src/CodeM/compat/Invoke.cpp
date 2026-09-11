#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Scalar.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include <type_traits>

#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesTape.h"
#include "CodeM/native/cpu/ConcatStackKernels.h"
#include "CodeM/native/cpu/AdvancedIndexingKernels.h"
#include "CodeM/native/cpu/GatherScatterKernels.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "CodeM/native/cpu/SignalDistanceKernels.h"
#include "CodeM/native/cpu/SortingKernels.h"
#include "CodeM/native/mps/Matmul.h"
#include "CodeM/native/mps/Sorting.h"
#include "CodeM/native/mps/Stochastic.h"
#include "CodeM/native/cuda/SelectionOps.h"
#include "CodeM/native/cuda/MatmulOps.h"
#include "CodeM/native/cuda/StochasticOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"
#include "pluss/tils/aux/rng_seed.h"

#include <sstream>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <functional>
#include <numeric>
#include <limits>
#include <optional>
#include <complex>
#include <random>
#include <iostream>
#include <unordered_map>

namespace pluss::codem::native::cpu {
Series matmul(const Series& a, const Series& b);
Series index_select(const Series& a, std::int64_t dim, const Series& index);
}

namespace pluss::codem::_compat {

using pluss::codem::warn;

constexpr double kPi = 3.14159265358979323846;

Series invoke(std::string_view op, const Series& self, ArgView args_view) {
  Args args(args_view.begin(), args_view.end());
  return invoke_any(op, self, args);
}

Series& invoke_(std::string_view op, Series& self, ArgView args_view) {
  Args args(args_view.begin(), args_view.end());
  return invoke_any_(op, self, args);
}

Series& invoke_(std::string_view op, const Series& self, ArgView args_view) {
  Args args(args_view.begin(), args_view.end());
  return invoke_any_(op, mut_(self), args);
}


static std::string sig(std::string_view op, const Args& args) {
  std::ostringstream os;
  os << op << "(";
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i) os << ", ";
    const auto& v = args[i].v;
    if (std::holds_alternative<const Series*>(v)) os << "Series";
    else if (std::holds_alternative<Arg::SeriesList>(v)) os << "Series[]";
    else if (std::holds_alternative<Scalar>(v)) os << "Scalar";
    else if (std::holds_alternative<double>(v)) os << "double";
    else if (std::holds_alternative<std::int64_t>(v)) os << "int64";
    else if (std::holds_alternative<bool>(v)) os << "bool";
    else if (std::holds_alternative<std::string>(v)) os << "string";
    else if (std::holds_alternative<Arg::IntList>(v)) os << "int64[]";
    else if (std::holds_alternative<std::monostate>(v)) os << "None";
    else os << "?";
  }
  os << ")";
  return os.str();
}

namespace {
std::atomic<bool>& strict_unrouted_flag_() {
  // Default: a call that reaches the generic unrouted backstop on CPU throws.
  // A generated method overload with no route would otherwise return the
  // receiver unchanged (a silent identity), which is indistinguishable from a
  // correct no-op and acquires a wrong identity derivative under autograd.
  // Set env PLUSS_STRICT_UNROUTED=0 to restore the old warn-and-passthrough
  // behavior during migration of a specific call site.
  static std::atomic<bool> flag{[] {
    const char* e = std::getenv("PLUSS_STRICT_UNROUTED");
    if (e == nullptr || e[0] == '\0') return true;
    return e[0] != '0';
  }()};
  return flag;
}
}  // namespace

void set_strict_unrouted(bool enabled) noexcept { strict_unrouted_flag_().store(enabled); }
bool strict_unrouted() noexcept { return strict_unrouted_flag_().load(); }

static Series not_impl(std::string_view op, const Args& args, const Series& self) {
  if (self.platform() != Platform::CPU) {
    throw_error(
        std::string("Unrouted op on non-CPU platform is disallowed: ") + sig(op, args) +
        " (platform=" + (self.platform() == Platform::MPS ? "MPS" : self.platform() == Platform::CUDA ? "CUDA" : "UNKNOWN") +
        "). Either implement backend kernel or move Series to CPU explicitly.");
  }
  if (strict_unrouted()) {
    throw_error(
        std::string("Unrouted op on CPU (strict mode): ") + sig(op, args) +
        " has no route to a kernel; it would otherwise return the receiver "
        "unchanged (a silent identity). Implement it or remove the call.");
  }
  warn(std::string("Unrouted op fallback: ") + sig(op, args));
  return self;
}

static Series& not_impl_inplace(std::string_view op, const Args& args, Series& self) {
  if (self.platform() != Platform::CPU) {
    throw_error(
        std::string("Unrouted in-place op on non-CPU platform is disallowed: ") + sig(op, args) +
        " (platform=" + (self.platform() == Platform::MPS ? "MPS" : self.platform() == Platform::CUDA ? "CUDA" : "UNKNOWN") +
        "). Either implement backend kernel or move Series to CPU explicitly.");
  }
  if (strict_unrouted()) {
    throw_error(
        std::string("Unrouted in-place op on CPU (strict mode): ") + sig(op, args) +
        " has no route to a kernel; it would otherwise be a silent no-op.");
  }
  warn(std::string("Unrouted in-place op fallback: ") + sig(op, args));
  return self;
}

static const Series& as_series(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<const Series*>(a.v), "Expected Series");
  return *std::get<const Series*>(a.v);
}

static Arg::SeriesList as_series_list(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<Arg::SeriesList>(a.v), "Expected Series list");
  return std::get<Arg::SeriesList>(a.v);
}

static void validate_series_arg_platforms(std::string_view op, const Series& self, const Args& args) {
  for (const auto& arg : args) {
    if (!std::holds_alternative<const Series*>(arg.v)) {
      if (std::holds_alternative<Arg::SeriesList>(arg.v)) {
        for (const auto& other : as_series_list(arg)) {
          ops_internal::require_same_platform(self, other, op);
        }
      }
      continue;
    }
    const Series& other = *std::get<const Series*>(arg.v);
    ops_internal::require_same_platform(self, other, op);
  }
}

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    std::int64_t storage_offset_elems);

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides);

static void bind_triangular_mask_backward_(
    const Series& out,
    const Series& self,
    std::int64_t diagonal,
    bool upper,
    const char* op_name);

static Series diagonal_scatter_impl(const Series& self, const Series& src, std::int64_t diagonal);

static std::vector<std::int64_t> leading_batch_dims_(const Series& s) {
  PLUSS_CHECK(s.dim() >= 2, "matmul: expected rank >= 2 for batched path");
  return std::vector<std::int64_t>(s.sizes().begin(), s.sizes().end() - 2);
}

static std::vector<std::int64_t> prepend_ones_(std::span<const std::int64_t> shape, std::size_t count) {
  std::vector<std::int64_t> out;
  out.reserve(count + shape.size());
  out.insert(out.end(), count, 1);
  out.insert(out.end(), shape.begin(), shape.end());
  return out;
}

static std::int64_t product_shape_(std::span<const std::int64_t> shape) {
  if (shape.empty()) return 1;
  std::int64_t p = 1;
  for (const auto v : shape) p *= v;
  return p;
}

static Series mps_matmul_batched_dispatch_(const Series& a, const Series& b) {
  const Series aa0 = a.is_contiguous() ? a : a.contiguous();
  const Series bb0 = b.is_contiguous() ? b : b.contiguous();

  // Vector/matrix promotion follows torch semantics.
  if (aa0.dim() == 1 && bb0.dim() == 1) {
    const Series mm = mps_matmul_batched_dispatch_(aa0.unsqueeze(0), bb0.unsqueeze(-1));
    return mm.reshape({});
  }
  if (aa0.dim() == 1) {
    return mps_matmul_batched_dispatch_(aa0.unsqueeze(0), bb0).squeeze(0);
  }
  if (bb0.dim() == 1) {
    return mps_matmul_batched_dispatch_(aa0, bb0.unsqueeze(-1)).squeeze(-1);
  }

  PLUSS_CHECK(aa0.dim() >= 2 && bb0.dim() >= 2, "matmul: MPS path expects rank>=2 inputs");
  PLUSS_CHECK(aa0.scalar_type() == bb0.scalar_type(), "matmul: MPS path expects matching dtypes");

  const std::int64_t m = aa0.sizes()[aa0.dim() - 2];
  const std::int64_t k = aa0.sizes()[aa0.dim() - 1];
  const std::int64_t k2 = bb0.sizes()[bb0.dim() - 2];
  const std::int64_t n = bb0.sizes()[bb0.dim() - 1];
  PLUSS_CHECK(k == k2, "matmul: shape mismatch (inner dimensions)");

  const auto a_batch = leading_batch_dims_(aa0);
  const auto b_batch = leading_batch_dims_(bb0);
  const auto out_batch = SeriesIterator::broadcast_shape(
      std::span<const std::int64_t>(a_batch.data(), a_batch.size()),
      std::span<const std::int64_t>(b_batch.data(), b_batch.size()));

  std::vector<std::int64_t> out_shape = out_batch;
  out_shape.push_back(m);
  out_shape.push_back(n);
  if (product_shape_(std::span<const std::int64_t>(out_shape.data(), out_shape.size())) == 0) {
    return Series::empty(out_shape, aa0.scalar_type(), Platform::MPS);
  }

  const std::size_t out_batch_rank = out_batch.size();
  const std::size_t a_batch_rank = static_cast<std::size_t>(aa0.dim() - 2);
  const std::size_t b_batch_rank = static_cast<std::size_t>(bb0.dim() - 2);

  const std::vector<std::int64_t> a_view_shape =
      prepend_ones_(std::span<const std::int64_t>(aa0.sizes().data(), aa0.sizes().size()), out_batch_rank - a_batch_rank);
  const std::vector<std::int64_t> b_view_shape =
      prepend_ones_(std::span<const std::int64_t>(bb0.sizes().data(), bb0.sizes().size()), out_batch_rank - b_batch_rank);

  Series aa = (std::vector<std::int64_t>(aa0.sizes().begin(), aa0.sizes().end()) == a_view_shape)
                  ? aa0
                  : aa0.reshape(a_view_shape);
  Series bb = (std::vector<std::int64_t>(bb0.sizes().begin(), bb0.sizes().end()) == b_view_shape)
                  ? bb0
                  : bb0.reshape(b_view_shape);

  std::vector<std::int64_t> a_target = out_batch;
  a_target.push_back(m);
  a_target.push_back(k);
  std::vector<std::int64_t> b_target = out_batch;
  b_target.push_back(k);
  b_target.push_back(n);

  if (std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end()) != a_target) {
    aa = aa.expand(a_target, true).contiguous();
  }
  if (std::vector<std::int64_t>(bb.sizes().begin(), bb.sizes().end()) != b_target) {
    bb = bb.expand(b_target, true).contiguous();
  }

  const std::int64_t batch = product_shape_(std::span<const std::int64_t>(out_batch.data(), out_batch.size()));
  if (batch == 1) {
    return native::mps::matmul(
        aa.reshape({m, k}).contiguous(),
        bb.reshape({k, n}).contiguous());
  }

  const Series aa_flat = aa.reshape({batch, m, k}).contiguous();
  const Series bb_flat = bb.reshape({batch, k, n}).contiguous();
  std::vector<Series> pages;
  pages.reserve(static_cast<std::size_t>(batch));
  for (std::int64_t i = 0; i < batch; ++i) {
    pages.push_back(native::mps::matmul(
        aa_flat.select(0, i).contiguous(),
        bb_flat.select(0, i).contiguous()));
  }
  PLUSS_CHECK(!pages.empty(), "matmul: internal MPS batched pages unexpectedly empty");
  Series out_batched = pages.front();
  if (pages.size() > 1) {
    std::vector<Series> tails;
    tails.reserve(pages.size() - 1);
    for (std::size_t i = 1; i < pages.size(); ++i) tails.push_back(pages[i]);
    out_batched = out_batched.pile(tails, 0);
  } else {
    out_batched = out_batched.unsqueeze(0);
  }
  return out_batched.reshape(out_shape);
}

static Series matmul_dispatch_(const Series& a, const Series& b) {
  ops_internal::require_same_platform(a, b, "matmul");
  if (a.platform() == Platform::MPS) {
    const Series aa = a.is_contiguous() ? a : a.contiguous();
    const Series bb = b.is_contiguous() ? b : b.contiguous();
    const auto mps_matmul_cpu_bridge_enabled = []() noexcept {
      const char* raw = std::getenv("PLUSS_MPS_MATMUL_CPU_BRIDGE");
      if (raw == nullptr || *raw == '\0') return true;
      auto eq_lit = [&](const char* lit) noexcept {
        const char* x = raw;
        const char* y = lit;
        while (*x != '\0' && *y != '\0') {
          if (std::tolower(static_cast<unsigned char>(*x)) !=
              std::tolower(static_cast<unsigned char>(*y))) {
            return false;
          }
          ++x;
          ++y;
        }
        return *x == '\0' && *y == '\0';
      };
      return eq_lit("1") || eq_lit("true") || eq_lit("yes") || eq_lit("on");
    }();
    if (mps_matmul_cpu_bridge_enabled &&
        aa.dim() == 2 &&
        bb.dim() == 2 &&
        bb.sizes()[1] == 1 &&
        aa.scalar_type() != ScalarType::Complex64 &&
        aa.scalar_type() != ScalarType::Complex128) {
      return native::cpu::matmul(
                 aa.to(Platform::CPU, aa.scalar_type()),
                 bb.to(Platform::CPU, bb.scalar_type()))
          .to(Platform::MPS, aa.scalar_type());
    }
    if (native::mps::supports_matmul(aa, bb)) {
      return native::mps::matmul(aa, bb);
    }
    return mps_matmul_batched_dispatch_(aa, bb);
  }
  if (a.platform() == Platform::CUDA) {
    if (auto cublas_out = native::cuda::try_cublas_matmul(a, b); cublas_out.has_value()) {
      return std::move(*cublas_out);
    }
    const bool native_enabled = native::cuda::native_matmul_enabled();
    const bool native_strict = native::cuda::native_matmul_strict();
    const bool route_supported = native::cuda::supports_matmul(a, b);
    if (native_enabled && route_supported) {
      throw_error("matmul: CUDA native matmul route declared but kernel is not implemented yet");
    }
    if (native_strict) {
      throw_error("matmul: strict native CUDA route requested but kernel is unsupported");
    }
    if (native::cuda::cpu_bridge_enabled()) {
      return native::cpu::matmul(a, b);
    }
    throw_error(
        "matmul: CUDA path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
  }
  return native::cpu::matmul(a, b);
}

static Series matmul_backprop_dispatch_(
    const Series& lhs,
    const Series& rhs,
    Platform out_platform,
    ScalarType out_dtype) {
  ops_internal::require_same_platform(lhs, rhs, "matmul(backprop)");
  Series out = matmul_dispatch_(lhs, rhs);
  if (out.scalar_type() != out_dtype) {
    out = out.to(out_dtype);
  }
  if (out.platform() != out_platform) {
    out = out.to(out_platform);
  }
  return out;
}

static Series reduce_broadcasted_like_(const Series& grad, const Series& like, std::string_view op_name) {
  std::vector<std::int64_t> grad_shape(grad.sizes().begin(), grad.sizes().end());
  std::vector<std::int64_t> like_shape(like.sizes().begin(), like.sizes().end());
  if (grad_shape == like_shape) {
    return grad;
  }

  PLUSS_CHECK(
      grad_shape.size() >= like_shape.size(),
      std::string(op_name) + ": broadcast reduction rank mismatch");

  const std::size_t prefix = grad_shape.size() - like_shape.size();
  std::vector<std::int64_t> aligned_like(grad_shape.size(), 1);
  for (std::size_t i = 0; i < like_shape.size(); ++i) {
    aligned_like[prefix + i] = like_shape[i];
  }

  Series out = grad;
  for (std::size_t i = 0; i < grad_shape.size(); ++i) {
    const auto gd = grad_shape[i];
    const auto ld = aligned_like[i];
    if (ld == gd) continue;
    PLUSS_CHECK(
        ld == 1,
        std::string(op_name) + ": unsupported non-broadcast matmul sensitivity reduction");
    out = out.sum(static_cast<std::int64_t>(i), /*keepdim=*/true);
  }

  if (std::vector<std::int64_t>(out.sizes().begin(), out.sizes().end()) != like_shape) {
    out = out.reshape(like_shape);
  }
  if (out.scalar_type() != like.scalar_type()) out = out.to(like.scalar_type());
  if (out.platform() != like.platform()) out = out.to(like.platform());
  return out;
}

static std::pair<Series, bool> promote_lhs_matmul_operand_(const Series& a) {
  if (a.dim() == 1) {
    return {a.unsqueeze(0), true};
  }
  return {a, false};
}

static std::pair<Series, bool> promote_rhs_matmul_operand_(const Series& b) {
  if (b.dim() == 1) {
    return {b.unsqueeze(-1), true};
  }
  return {b, false};
}

static bool complex_matmul_dtype_(ScalarType st) noexcept {
  return st == ScalarType::Complex64 || st == ScalarType::Complex128;
}

static Series matmul_adjoint_(const Series& x) {
  Series xt = x.transpose(-2, -1);
  return complex_matmul_dtype_(x.scalar_type()) ? ops_internal::conj(xt) : xt;
}

static Series promote_out_sensitivity_for_matmul_(
    const Series& out_sens,
    bool lhs_was_vec,
    bool rhs_was_vec) {
  if (lhs_was_vec && rhs_was_vec) {
    const std::array<std::int64_t, 2> scalar_mat_shape{{1, 1}};
    return out_sens.reshape(std::span<const std::int64_t>(scalar_mat_shape.data(), scalar_mat_shape.size()));
  }
  if (lhs_was_vec) {
    return out_sens.unsqueeze(-2);
  }
  if (rhs_was_vec) {
    return out_sens.unsqueeze(-1);
  }
  return out_sens;
}

static Series matmul_with_tape_(const Series& a, const Series& b) {
  Series out = matmul_dispatch_(a, b);
  if (!series_tape::recording_enabled()) return out;
  const bool ta = series_tape::tracks_slope(a);
  const bool tb = series_tape::tracks_slope(b);
  static const bool debug_matmul_tape = []() {
    const char* raw = std::getenv("PLUSS_DEBUG_MATMUL_TAPE");
    if (!raw) return false;
    return std::strcmp(raw, "1") == 0 || std::strcmp(raw, "true") == 0 || std::strcmp(raw, "TRUE") == 0;
  }();
  static int debug_matmul_count = 0;
  if (debug_matmul_tape && a.platform() == Platform::MPS && debug_matmul_count < 200) {
    ++debug_matmul_count;
    std::cerr << "[matmul-tape] #" << debug_matmul_count
              << " a_dim=" << a.dim()
              << " b_dim=" << b.dim()
              << " a_tracks=" << (ta ? 1 : 0)
              << " b_tracks=" << (tb ? 1 : 0)
              << " a_shape=";
    for (std::size_t i = 0; i < a.sizes().size(); ++i) {
      if (i) std::cerr << "x";
      std::cerr << a.sizes()[i];
    }
    std::cerr << " b_shape=";
    for (std::size_t i = 0; i < b.sizes().size(); ++i) {
      if (i) std::cerr << "x";
      std::cerr << b.sizes()[i];
    }
    std::cerr << "\n";
  }
  if (!ta && !tb) return out;
  // Reverse path currently supports vector/matrix/batched-matrix matmul semantics.
  if (a.dim() < 1 || b.dim() < 1) {
    throw_error(
        "matmul: reverse-mode sensitivity currently supports tracked rank>=1 matmul inputs only");
  }

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "matmul";
  link->parents = {a, b};
  link->propagate = [a, b, ta, tb](const Series& out_sens) {
    const auto [a_promoted, a_was_vec] = promote_lhs_matmul_operand_(a);
    const auto [b_promoted, b_was_vec] = promote_rhs_matmul_operand_(b);
    const Series g_promoted = promote_out_sensitivity_for_matmul_(out_sens, a_was_vec, b_was_vec);

    if (ta) {
      const Series bt = matmul_adjoint_(b_promoted);
      Series ga = matmul_backprop_dispatch_(
          g_promoted,
          bt,
          a_promoted.platform(),
          a_promoted.scalar_type());
      ga = reduce_broadcasted_like_(ga, a_promoted, "matmul");
      if (a_was_vec) {
        ga = ga.squeeze(0);
      }
      series_tape::accumulate_sensitivity(a, ga);
    }
    if (tb) {
      const Series at = matmul_adjoint_(a_promoted);
      Series gb = matmul_backprop_dispatch_(
          at,
          g_promoted,
          b_promoted.platform(),
          b_promoted.scalar_type());
      gb = reduce_broadcasted_like_(gb, b_promoted, "matmul");
      if (b_was_vec) {
        gb = gb.squeeze(-1);
      }
      series_tape::accumulate_sensitivity(b, gb);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}


static double scalar_to_double(const Scalar& s) {
  return std::visit(
      [](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) return v;
        else if constexpr (std::is_same_v<T, bool>) return v ? 1.0 : 0.0;
        else if constexpr (std::is_same_v<T, std::int64_t>) return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, std::uint64_t>) return static_cast<double>(v);
        else return 0.0;
      },
      s.storage());
}

static std::int64_t scalar_to_i64(const Scalar& s) {
  return std::visit(
      [](const auto& v) -> std::int64_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::int64_t>) return v;
        else if constexpr (std::is_same_v<T, std::uint64_t>) return static_cast<std::int64_t>(v);
        else if constexpr (std::is_same_v<T, bool>) return v ? 1 : 0;
        else if constexpr (std::is_same_v<T, double>) return static_cast<std::int64_t>(v);
        else return 0;
      },
      s.storage());
}

static double as_double(const Arg& a) {
  if (std::holds_alternative<double>(a.v)) return std::get<double>(a.v);
  if (std::holds_alternative<std::int64_t>(a.v)) return static_cast<double>(std::get<std::int64_t>(a.v));
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_double(std::get<Scalar>(a.v));
  PLUSS_CHECK(false, "Expected numeric scalar");
  return 0.0;
}

static std::int64_t as_i64(const Arg& a) {
  if (std::holds_alternative<std::int64_t>(a.v)) return std::get<std::int64_t>(a.v);
  if (std::holds_alternative<double>(a.v)) return static_cast<std::int64_t>(std::get<double>(a.v));
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_i64(std::get<Scalar>(a.v));
  PLUSS_CHECK(false, "Expected int64 scalar");
  return 0;
}

static bool as_bool(const Arg& a) {
  if (std::holds_alternative<bool>(a.v)) return std::get<bool>(a.v);
  if (std::holds_alternative<std::int64_t>(a.v)) return std::get<std::int64_t>(a.v) != 0;
  if (std::holds_alternative<double>(a.v)) return std::get<double>(a.v) != 0.0;
  if (std::holds_alternative<Scalar>(a.v)) return scalar_to_double(std::get<Scalar>(a.v)) != 0.0;
  PLUSS_CHECK(false, "Expected bool");
  return false;
}


static std::span<const std::int64_t> as_intlist(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<Arg::IntList>(a.v), "Expected int64 list");
  return std::get<Arg::IntList>(a.v);
}

static std::string_view as_string(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<std::string>(a.v), "Expected string");
  return std::get<std::string>(a.v);
}

static bool is_numeric_scalar_arg(const Arg& a) {
  return std::holds_alternative<double>(a.v) ||
         std::holds_alternative<std::int64_t>(a.v) ||
         std::holds_alternative<Scalar>(a.v);
}

static const Series* as_optional_series_ptr(const Arg& a) {
  if (std::holds_alternative<std::monostate>(a.v)) return nullptr;
  if (std::holds_alternative<const Series*>(a.v)) return std::get<const Series*>(a.v);
  PLUSS_CHECK(false, "Expected optional Series");
  return nullptr;
}

static Series& as_mutable_series_ref(const Arg& a) {
  PLUSS_CHECK(std::holds_alternative<const Series*>(a.v), "Expected mutable Series reference");
  return *const_cast<Series*>(std::get<const Series*>(a.v));
}

static double as_numeric_or_series_scalar_double(const Arg& a) {
  if (is_numeric_scalar_arg(a)) return as_double(a);
  if (std::holds_alternative<const Series*>(a.v)) {
    const Series& s = as_series(a);
    PLUSS_CHECK(s.defined() && s.numel() == 1, "Expected scalar Series");
    return s.to(ScalarType::Float64).item<double>();
  }
  PLUSS_CHECK(false, "Expected numeric scalar or scalar Series");
  return 0.0;
}

static Series scalar_like(const Series& self, double value) {
  const std::array<std::int64_t, 1> one{{1}};
  // Force construction on the target platform scope to avoid leaking a prior
  // scope (e.g. MPS) into CPU-side scalar temporaries.
  ops_internal::PlatformScope scope{self.platform()};
  return Series::full(std::span<const std::int64_t>(one.data(), one.size()), value, self.scalar_type())
      .to(self.platform());
}

static Series add_scalar(const Series& a, double s) { return ops_internal::add(a, scalar_like(a, s)); }
static Series sub_scalar(const Series& a, double s) { return ops_internal::sub(a, scalar_like(a, s)); }
static Series mul_scalar(const Series& a, double s) { return ops_internal::mul(a, scalar_like(a, s)); }
static Series div_scalar(const Series& a, double s) { return ops_internal::div(a, scalar_like(a, s)); }
static Series pow_scalar(const Series& a, double s) { return ops_internal::pow(a, s); }

static Series& assign_inplace_preserve_storage_(Series& self, const Series& value) {
  if (!self.defined() || !value.defined()) {
    self = value;
    return self;
  }
  const bool same_platform = self.platform() == value.platform();
  const bool same_dtype = self.scalar_type() == value.scalar_type();
  const bool same_rank = self.sizes().size() == value.sizes().size();
  bool same_shape = same_rank;
  if (same_shape) {
    for (std::size_t i = 0; i < self.sizes().size(); ++i) {
      if (self.sizes()[i] != value.sizes()[i]) {
        same_shape = false;
        break;
      }
    }
  }
  PLUSS_CHECK(same_platform, "copy_: source and destination must be on the same platform in this path");
  PLUSS_CHECK(same_dtype, "copy_: dtype conversion must be completed before in-place copy");
  PLUSS_CHECK(same_shape, "copy_: source must have the same shape as destination in this path");
  
  if (self.is_contiguous() && value.is_contiguous() && self.itemsize() == value.itemsize()) {
    const auto nbytes = static_cast<std::size_t>(self.numel()) * static_cast<std::size_t>(self.itemsize());
    if (nbytes > 0) {
      std::memcpy(self.raw_mutable_data(), value.raw_data(), nbytes);
    }
    return self;
  }
  SeriesIterator it = SeriesIterator::unary(value, self);
  it.copy_out_from_in(value, self);
  return self;
}

static Series& assign_stochastic_inplace_(Series& self, const Series& value) {
  assign_inplace_preserve_storage_(self, value);
  if (self.defined()) self.storage().bump_version();
  return self;
}

// Result of an in-place (`_`-suffixed) op. When `value` has the same shape,
// dtype, and platform as `self` -- the case for essentially every `x.op_()`
// where the op does not change the tensor's type or geometry -- the values are
// written back into `self`'s existing storage so that aliases (`auto y = x;`)
// observe the mutation and `is_set_to` identity is preserved. Only when the
// result genuinely changes shape/dtype/platform (a comparison `_` producing
// Bool, a shape-changing `squeeze_`, a cross-platform move) does `self` rebind
// to the new buffer, exactly as before.
static Series& inplace_result_(Series& self, const Series& value) {
  if (self.defined() && value.defined() &&
      self.platform() == value.platform() &&
      self.scalar_type() == value.scalar_type() &&
      self.is_same_size(value)) {
    return assign_inplace_preserve_storage_(self, value);
  }
  self = value;
  return self;
}

static Series bool_scalar(bool value) {
  Series out = Series::zeros({}, ScalarType::Bool);
  *reinterpret_cast<std::uint8_t*>(out.raw_data()) = value ? 1 : 0;
  return out;
}

static Series i64_scalar(std::int64_t value) {
  Series out = Series::zeros({}, ScalarType::Int64);
  *reinterpret_cast<std::int64_t*>(out.raw_data()) = value;
  return out;
}

static Series i64_vector(std::span<const std::int64_t> values) {
  const std::array<std::int64_t, 1> shape{{static_cast<std::int64_t>(values.size())}};
  Series out = Series::empty(std::span<const std::int64_t>(shape.data(), shape.size()), ScalarType::Int64);
  auto* p = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(values.size()); ++i) p[i] = values[(std::size_t)i];
  return out;
}

static Series i64_vector_on(std::span<const std::int64_t> values, Platform platform) {
  return i64_vector(values).to(platform);
}

static std::vector<Series> normalize_index_list_for_put_(
    const Arg::SeriesList& indices,
    const Series& like) {
  std::vector<Series> out;
  out.reserve(indices.size());
  for (const auto& idx_raw : indices) {
    if (!idx_raw.defined()) {
      out.push_back(Series::undefined());
      continue;
    }
    Series idx = idx_raw;
    if (idx.platform() != like.platform()) idx = idx.to(like.platform());
    if (idx.scalar_type() != ScalarType::Bool && idx.scalar_type() != ScalarType::Int64) {
      idx = idx.to(ScalarType::Int64);
    }
    out.push_back(std::move(idx));
  }
  return out;
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

static bool supports_put_autodiff_1d_(
    const Series& self,
    const Series& index,
    const Series& src) {
  if (self.dim() != 1) return false;
  if (index.dim() != 1) return false;
  if (!is_integer_index_dtype_(index.scalar_type())) return false;
  if (index.numel() <= 0) return false;
  return (src.numel() == 1 || src.numel() == index.numel());
}

static bool shares_storage_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  return a.storage().handle().get() == b.storage().handle().get();
}

static Series alias_view_on_(const Series& base, const Series& view_like) {
  PLUSS_CHECK(base.defined() && view_like.defined(), "copy_ backward: expected defined Series");
  PLUSS_CHECK(
      base.scalar_type() == view_like.scalar_type(),
      "copy_ backward: expected matching dtype for base/view aliasing");
  const auto storage_offset_elems =
      view_like.storage_offset() / std::max<std::int64_t>(1, view_like.itemsize());
  return Series::from_storage(
      base.storage(),
      storage_offset_elems,
      std::vector<std::int64_t>(view_like.sizes().begin(), view_like.sizes().end()),
      std::vector<std::int64_t>(view_like.strides().begin(), view_like.strides().end()),
      base.scalar_type());
}

static Series find_root_alias_for_view_(const Series& view) {
  Series cur = view;
  constexpr int kMaxAliasHops = 64;
  for (int hop = 0; hop < kMaxAliasHops; ++hop) {
    const auto link = series_tape::source_link(cur);
    if (!link) break;
    if (link->parents.size() != 1) break;
    const Series& parent = link->parents.front();
    if (!parent.defined()) break;
    if (!shares_storage_(parent, cur)) break;
    cur = parent;
  }
  return cur;
}

static Series make_pre_copy_parent_snapshot_(const Series& root_before) {
  const auto root_link = series_tape::source_link(root_before);
  if (!root_link) return Series::undefined();

  // Snapshot must own stable storage so subsequent in-place writes on root do
  // not invalidate saved-version checks for historical links.
  Series snapshot = root_before.clone();
  series_tape::set_tracks_slope(snapshot, true);
  auto snapshot_link = std::make_shared<series_tape::SourceLink>(*root_link);
  series_tape::bind_source_link(snapshot, std::move(snapshot_link));
  return snapshot;
}

static Series make_inplace_bridge_input_snapshot_(const Series& self) {
  Series snapshot = Series::empty(self.sizes(), self.scalar_type()).to(self.platform());
  assign_inplace_preserve_storage_(snapshot, self);
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) {
    return snapshot;
  }
  const Series parent_snapshot = make_pre_copy_parent_snapshot_(self);
  series_tape::set_tracks_slope(snapshot, true);
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "inplace_bridge_input";
  if (parent_snapshot.defined()) {
    link->parents = {parent_snapshot};
  }
  link->propagate = [self, parent_snapshot](const Series& out_sens) {
    Series sens = out_sens;
    if (sens.platform() != self.platform()) sens = sens.to(self.platform());
    if (sens.scalar_type() != self.scalar_type()) sens = sens.to(self.scalar_type());
    if (std::vector<std::int64_t>(sens.sizes().begin(), sens.sizes().end()) !=
        std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end())) {
      sens = sens.reshape(std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end()));
    }
    if (parent_snapshot.defined()) {
      Series parent_sens = sens;
      if (parent_sens.platform() != parent_snapshot.platform()) {
        parent_sens = parent_sens.to(parent_snapshot.platform());
      }
      if (parent_sens.scalar_type() != parent_snapshot.scalar_type()) {
        parent_sens = parent_sens.to(parent_snapshot.scalar_type());
      }
      if (std::vector<std::int64_t>(parent_sens.sizes().begin(), parent_sens.sizes().end()) !=
          std::vector<std::int64_t>(parent_snapshot.sizes().begin(), parent_snapshot.sizes().end())) {
        parent_sens = parent_sens.reshape(
            std::vector<std::int64_t>(parent_snapshot.sizes().begin(), parent_snapshot.sizes().end()));
      }
      series_tape::accumulate_sensitivity(parent_snapshot, parent_sens);
      return;
    }
    series_tape::accumulate_sensitivity(self, sens);
  };
  series_tape::bind_source_link(snapshot, std::move(link));
  return snapshot;
}

struct InplaceBridgeArgs final {
  Args args;
  std::vector<Series> owned_views;
};

static InplaceBridgeArgs remap_inplace_bridge_args_to_snapshot_(
    const Args& args,
    const Series& self,
    const Series& snapshot) {
  InplaceBridgeArgs out;
  out.args.reserve(args.size());
  for (const auto& arg : args) {
    if (std::holds_alternative<const Series*>(arg.v)) {
      const Series& ref = *std::get<const Series*>(arg.v);
      if (shares_storage_(ref, self) && ref.scalar_type() == self.scalar_type()) {
        out.owned_views.push_back(alias_view_on_(snapshot, ref));
        out.args.emplace_back(out.owned_views.back());
        continue;
      }
      out.args.push_back(arg);
      continue;
    }
    if (std::holds_alternative<Arg::SeriesList>(arg.v)) {
      const auto& list = std::get<Arg::SeriesList>(arg.v);
      bool aliased = false;
      for (const auto& s : list) {
        if (s.defined() && shares_storage_(s, self)) {
          aliased = true;
          break;
        }
      }
      PLUSS_CHECK(
          !aliased,
          "in-place autodiff bridge: SeriesList argument aliases destination; "
          "use copy_/put_/index_put_ for aliased indexed writes");
      out.args.push_back(arg);
      continue;
    }
    out.args.push_back(arg);
  }
  return out;
}

static Series select_with_index_list_(const Series& self, const std::vector<Series>& indices) {
  std::vector<ExtractSpec> specs;
  specs.reserve(indices.size());
  for (const auto& idx : indices) {
    if (idx.defined()) {
      specs.emplace_back(idx);
    } else {
      specs.emplace_back(pluss::Slicing::SliceAll());
    }
  }
  return self.index(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
}

static void bind_put_backward_index_list_(
    const Series& out,
    const Series& self_before,
    const std::vector<Series>& indices_i64,
    const Series& src,
    bool accumulate) {
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "index_put_";
  link->parents = {self_before, src};
  link->propagate = [self_before, indices_i64, src, accumulate](const Series& out_sens) {
    Series sens = out_sens;
    if (sens.platform() != self_before.platform()) sens = sens.to(self_before.platform());
    if (sens.scalar_type() != self_before.scalar_type()) sens = sens.to(self_before.scalar_type());

    if (series_tape::tracks_slope(self_before)) {
      Series g_self = sens.clone();
      if (!accumulate) {
        Series zeros = Series::zeros({}, self_before.scalar_type()).to(self_before.platform());
        native::cpu::index_put_inplace(g_self, indices_i64, zeros, /*accumulate=*/false);
      }
      series_tape::accumulate_sensitivity(self_before, g_self);
    }

    if (series_tape::tracks_slope(src)) {
      Series g_src = select_with_index_list_(sens, indices_i64);
      if (src.numel() == 1) {
        g_src = g_src.sum().reshape(src.sizes());
      } else {
        PLUSS_CHECK(
            g_src.numel() == src.numel(),
            "index_put_ backward: source shape mismatch with indexed selection");
        g_src = g_src.reshape(src.sizes());
      }
      if (g_src.platform() != src.platform()) g_src = g_src.to(src.platform());
      if (g_src.scalar_type() != src.scalar_type()) g_src = g_src.to(src.scalar_type());
      series_tape::accumulate_sensitivity(src, g_src);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

static void bind_put_backward_1d_(
    const Series& out,
    const Series& self_before,
    const Series& index_i64,
    const Series& src,
    bool accumulate) {
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "index_put_";
  link->parents = {self_before, src};
  link->propagate = [self_before, index_i64, src, accumulate](const Series& out_sens) {
    Series sens = out_sens;
    if (sens.platform() != self_before.platform()) sens = sens.to(self_before.platform());
    if (sens.scalar_type() != self_before.scalar_type()) sens = sens.to(self_before.scalar_type());

    if (series_tape::tracks_slope(self_before)) {
      Series g_self = accumulate ? sens : sens.clone();
      if (!accumulate) {
        Series zeros = Series::zeros({index_i64.numel()}, self_before.scalar_type()).to(self_before.platform());
        auto put_fn = static_cast<Series (Series::*)(const Series&, const Series&, bool)>(&Series::put_);
        g_self = (g_self.*put_fn)(index_i64, zeros, /*accumulate=*/false);
      }
      series_tape::accumulate_sensitivity(self_before, g_self);
    }

    if (series_tape::tracks_slope(src)) {
      Series g_src = sens.take(index_i64);
      if (src.numel() == 1) {
        g_src = g_src.sum().reshape(src.sizes());
      } else {
        PLUSS_CHECK(
            src.numel() == index_i64.numel(),
            "index_put_ backward: source must be scalar or have the same length as index");
        g_src = g_src.reshape(src.sizes());
      }
      if (g_src.platform() != src.platform()) g_src = g_src.to(src.platform());
      if (g_src.scalar_type() != src.scalar_type()) g_src = g_src.to(src.scalar_type());
      series_tape::accumulate_sensitivity(src, g_src);
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series put_dispatch_copy_indices_(
    const Series& self,
    const Arg::SeriesList& indices,
    const Series& src,
    bool accumulate) {
  PLUSS_CHECK(!indices.empty(), "index_put: indices list cannot be empty");
  const bool track_self = series_tape::recording_enabled() && series_tape::tracks_slope(self);
  auto out = self.clone();
  series_tape::set_tracks_slope(out, false);

  const std::vector<Series> normalized_indices = normalize_index_list_for_put_(indices, out);
  Series val = src;
  if (val.platform() != out.platform()) val = val.to(out.platform());
  if (val.scalar_type() != out.scalar_type()) val = val.to(out.scalar_type());

  const bool use_flat_put_fast_path =
      out.dim() == 1 &&
      normalized_indices.size() == 1 &&
      normalized_indices.front().defined() &&
      is_integer_index_dtype_(normalized_indices.front().scalar_type());

  if (use_flat_put_fast_path) {
    auto put_fn = static_cast<Series (Series::*)(const Series&, const Series&, bool)>(&Series::put_);
    out = (out.*put_fn)(normalized_indices.front(), val, accumulate);
  } else {
    native::cpu::index_put_inplace(out, normalized_indices, val, accumulate);
  }

  const bool track_src = series_tape::recording_enabled() && series_tape::tracks_slope(src);
  if (series_tape::recording_enabled() && (track_self || track_src)) {
    if (use_flat_put_fast_path &&
        supports_put_autodiff_1d_(self, normalized_indices.front(), val)) {
      bind_put_backward_1d_(out, self, normalized_indices.front(), src, accumulate);
    } else {
      bind_put_backward_index_list_(out, self, normalized_indices, src, accumulate);
    }
  }
  return out;
}

static Series put_dispatch_copy_(
    const Series& self,
    const Series& index,
    const Series& src,
    bool accumulate) {
  const bool track_self = series_tape::recording_enabled() && series_tape::tracks_slope(self);
  auto out = self.clone();
  // Build value update on a detached working buffer; we manually re-attach
  // the autograd link for supported index_put_ patterns below.
  series_tape::set_tracks_slope(out, false);
  Series idx = index;
  if (idx.platform() != out.platform()) idx = idx.to(out.platform());
  if (idx.scalar_type() != ScalarType::Int64) idx = idx.to(ScalarType::Int64);
  Series val = src;
  if (val.platform() != out.platform()) val = val.to(out.platform());
  if (val.scalar_type() != out.scalar_type()) val = val.to(out.scalar_type());
  auto put_fn = static_cast<Series (Series::*)(const Series&, const Series&, bool)>(&Series::put_);
  out = (out.*put_fn)(idx, val, accumulate);

  const bool track_src = series_tape::recording_enabled() && series_tape::tracks_slope(src);
  if (series_tape::recording_enabled() && (track_self || track_src)) {
    if (supports_put_autodiff_1d_(self, idx, val)) {
      bind_put_backward_1d_(out, self, idx, src, accumulate);
    } else {
      // Generic index-list fallback keeps autograd connected for broader
      // index_put_ patterns (mirrors SeriesList path behavior).
      bind_put_backward_index_list_(out, self, std::vector<Series>{idx}, src, accumulate);
    }
  }
  return out;
}

static Series& put_dispatch_inplace_(
    Series& self,
    const Series& index,
    const Series& src,
    bool accumulate) {
  const bool keep_self_sensitivity =
      series_tape::recording_enabled() &&
      series_tape::tracks_slope(self) &&
      series_tape::is_leaf(self);
  Series idx = index;
  if (idx.platform() != self.platform()) idx = idx.to(self.platform());
  Series val = src;
  if (val.platform() != self.platform()) val = val.to(self.platform());
  if (val.scalar_type() != self.scalar_type()) val = val.to(self.scalar_type());
  // Route through functional copy path so autograd links are preserved.
  self = put_dispatch_copy_(self, idx, val, accumulate);
  if (keep_self_sensitivity) {
    series_tape::set_keep_sensitivity(self, true);
  }
  return self;
}

static Series& put_dispatch_inplace_indices_(
    Series& self,
    const Arg::SeriesList& indices,
    const Series& src,
    bool accumulate) {
  const bool keep_self_sensitivity =
      series_tape::recording_enabled() &&
      series_tape::tracks_slope(self) &&
      series_tape::is_leaf(self);
  self = put_dispatch_copy_indices_(self, indices, src, accumulate);
  if (keep_self_sensitivity) {
    series_tape::set_keep_sensitivity(self, true);
  }
  return self;
}

static Series& copy_dispatch_inplace_(Series& self, const Series& other) {
  const Series src_user = other;
  Series src = other;
  if (src.platform() != self.platform()) src = src.to(self.platform());
  if (src.scalar_type() != self.scalar_type()) src = src.to(self.scalar_type());

  const bool recording = series_tape::recording_enabled();
  const Series self_before = self;
  const Series root_before = find_root_alias_for_view_(self_before);
  const bool track_root = recording && series_tape::tracks_slope(root_before);
  const bool track_src = recording && series_tape::tracks_slope(src_user);
  Series root_parent_snapshot = Series::undefined();
  if (track_root) series_tape::set_tracks_slope(root_before, true);
  if (track_root) {
    root_parent_snapshot = make_pre_copy_parent_snapshot_(root_before);
  }
  if (track_src) series_tape::set_tracks_slope(src_user, true);

  assign_inplace_preserve_storage_(self, src);
  // copy_ is a true storage write; bump immediately so captured output version
  // matches post-mutation state when we bind the new source link.
  if (self.defined()) self.storage().bump_version();
  if (recording && (track_root || track_src)) {
    Series root_after = root_before;
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "copy_";
    if (root_parent_snapshot.defined()) {
      link->parents.push_back(root_parent_snapshot);
    }
    if (track_src) {
      link->parents.push_back(src_user);
    }
    link->propagate = [root_before,
                       self_before,
                       root_parent_snapshot,
                       src_user,
                       track_root,
                       track_src](const Series& out_sens) {
      Series sens = out_sens;
      if (sens.platform() != root_before.platform()) sens = sens.to(root_before.platform());
      if (sens.scalar_type() != root_before.scalar_type()) sens = sens.to(root_before.scalar_type());

      if (track_root && (root_parent_snapshot.defined() || track_src)) {
        // Masking (zeroing the overwritten region out of root_before's own
        // sensitivity) is only correct when there's an actual destination to
        // redirect that region's gradient to: earlier graph history
        // (root_parent_snapshot) or a tracked copy_ source (src_user). If
        // neither applies -- e.g. a freshly allocated leaf parameter
        // one-time-initialized via `weight_.copy_(untracked_init_values)`,
        // the established pattern across NodeK modules (Conv, Embedding,
        // MultiheadAttention, Transformer*) -- there is nowhere for the
        // masked-out gradient to go, so masking would silently discard real
        // sensitivity accumulated later from normal downstream use of
        // root_before. In that case root_before must keep behaving as a
        // plain leaf: leave its accumulated sensitivity untouched.
        Series g_root = sens.clone();
        Series g_view = alias_view_on_(g_root, self_before);
        const Series zeros =
            Series::zeros(g_view.sizes(), g_view.scalar_type()).to(g_view.platform());
        assign_inplace_preserve_storage_(g_view, zeros);
        if (root_parent_snapshot.defined()) {
          series_tape::accumulate_sensitivity(root_parent_snapshot, g_root);
        } else {
          // Leaf root with a tracked copy_ source: avoid self-cycle double
          // counting by replacing the current sensitivity with masked
          // pre-copy sensitivity.
          series_tape::clear_sensitivity(root_before);
          series_tape::accumulate_sensitivity(root_before, g_root);
        }
      }

      if (track_src) {
        Series g_src = alias_view_on_(sens, self_before).contiguous();
        if (src_user.numel() == 1) {
          g_src = g_src.sum().reshape(src_user.sizes());
        } else {
          PLUSS_CHECK(
              g_src.numel() == src_user.numel(),
              "copy_ backward: source shape mismatch with destination view");
          g_src = g_src.reshape(src_user.sizes());
        }
        if (g_src.platform() != src_user.platform()) g_src = g_src.to(src_user.platform());
        if (g_src.scalar_type() != src_user.scalar_type()) g_src = g_src.to(src_user.scalar_type());
        series_tape::accumulate_sensitivity(src_user, g_src);
      }
    };
    series_tape::bind_source_link(root_after, std::move(link));
    if (track_root && !root_parent_snapshot.defined()) {
      // Leaf-root copy_ writes should retain post-reverse sensitivity on the
      // destination tensor to match leaf gradient expectations.
      series_tape::set_keep_sensitivity(root_after, true);
    }
  }
  return self;
}

static Series make_expanded_gather_index_(
    const Series& idx_flat_i64,
    std::span<const std::int64_t> out_shape,
    std::int64_t dim,
    Platform target_platform) {
  PLUSS_CHECK(idx_flat_i64.scalar_type() == ScalarType::Int64,
              "index_select: expected flattened int64 index");
  const auto nd = static_cast<std::int64_t>(out_shape.size());
  PLUSS_CHECK(nd >= 1, "index_select: expected at least 1 dimension");
  PLUSS_CHECK(dim >= 0 && dim < nd, "index_select: dim out of range");

  Series idx = idx_flat_i64;
  if (idx.platform() != target_platform) idx = idx.to(target_platform);
  idx = idx.contiguous();
  // from_storage() currently materializes CPU views; build the broadcasted view
  // on CPU first, then move to the target backend.
  Series idx_cpu = (idx.platform() == Platform::CPU ? idx : idx.to(Platform::CPU)).contiguous();

  std::vector<std::int64_t> idx_shape(static_cast<std::size_t>(nd), 1);
  idx_shape[static_cast<std::size_t>(dim)] = idx.numel();
  PLUSS_CHECK(
      idx_shape[static_cast<std::size_t>(dim)] == out_shape[static_cast<std::size_t>(dim)],
      "index_select: internal index shape mismatch");
  // Build a broadcasted view directly from storage/strides to avoid routing
  // through compat expand() (which can recurse back into index_select paths).
  std::vector<std::int64_t> view_strides(static_cast<std::size_t>(nd), 0);
  std::int64_t running = 1;
  for (std::int64_t d = nd - 1; d >= 0; --d) {
    const auto du = static_cast<std::size_t>(d);
    view_strides[du] = running;
    running *= std::max<std::int64_t>(idx_shape[du], 1);
  }
  for (std::size_t d = 0; d < static_cast<std::size_t>(nd); ++d) {
    if (idx_shape[d] == 1 && out_shape[d] != 1) {
      view_strides[d] = 0;
    }
  }
  const auto base_off = idx_cpu.storage_offset() / std::max<std::int64_t>(1, idx_cpu.itemsize());
  Series view_cpu = Series::from_storage(
      idx_cpu.storage(),
      base_off,
      std::vector<std::int64_t>(out_shape.begin(), out_shape.end()),
      view_strides,
      idx_cpu.scalar_type()).contiguous();
  return target_platform == Platform::CPU ? view_cpu : view_cpu.to(target_platform).contiguous();
}

static Series index_select_dispatch_(const Series& self, std::int64_t dim, const Series& index) {
  auto idx = index;
  if (idx.platform() != self.platform()) idx = idx.to(self.platform());
  if (idx.scalar_type() != ScalarType::Int64) idx = idx.to(ScalarType::Int64);
  idx = idx.flatten(0, -1).contiguous();

  const auto nd = self.dim();
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, "index_select: dim out of range");

  std::vector<std::int64_t> out_shape(self.sizes().begin(), self.sizes().end());
  out_shape[static_cast<std::size_t>(dim)] = idx.numel();

  const auto idx_expanded = make_expanded_gather_index_(
      idx,
      std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
      dim,
      self.platform());
  Series out = ops_internal::gather(self, dim, idx_expanded);

  // Optional MPS fallback hook: rebind index_select backward to a CPU scatter
  // route only when explicitly requested. Default path stays fully device-native.
  const auto use_mps_cpu_index_select_backward = []() noexcept {
    const char* raw = std::getenv("PLUSS_MPS_INDEX_SELECT_CPU_BACKWARD");
    if (raw == nullptr || *raw == '\0') return false;
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
    return eq_lit("1") || eq_lit("true") || eq_lit("yes") || eq_lit("on");
  }();
  if (self.platform() == Platform::MPS &&
      use_mps_cpu_index_select_backward &&
      series_tape::recording_enabled() &&
      series_tape::tracks_slope(self)) {
    Series idx_cpu = idx;
    if (idx_cpu.platform() != Platform::CPU) idx_cpu = idx_cpu.to(Platform::CPU);
    if (idx_cpu.scalar_type() != ScalarType::Int64) idx_cpu = idx_cpu.to(ScalarType::Int64);
    idx_cpu = idx_cpu.contiguous();
    const Series idx_expanded_cpu = make_expanded_gather_index_(
        idx_cpu,
        std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
        dim,
        Platform::CPU)
                                        .contiguous();

    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "index_select";
    link->parents = {self};
    link->propagate = [self, dim, idx_expanded_cpu](const Series& out_sens) {
      Series sens = out_sens;
      if (sens.scalar_type() != self.scalar_type()) sens = sens.to(self.scalar_type());
      if (sens.platform() != Platform::CPU) sens = sens.to(Platform::CPU, self.scalar_type());
      sens = sens.contiguous();
      Series base = Series::zeros(self.sizes(), self.scalar_type()).to(Platform::CPU);
      Series grad = ops_internal::scatter_add(base, dim, idx_expanded_cpu, sens);
      if (self.platform() != Platform::CPU) grad = grad.to(self.platform(), self.scalar_type()).contiguous();
      series_tape::accumulate_sensitivity(self, grad);
    };
    series_tape::clear_source_link(out);
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static bool is_floating_dtype(ScalarType st) {
  return st == ScalarType::Float16 || st == ScalarType::BFloat16 ||
         st == ScalarType::Float32 || st == ScalarType::Float64;
}

static bool is_complex_dtype(ScalarType st) {
  return st == ScalarType::Complex64 || st == ScalarType::Complex128;
}

static bool is_signed_dtype(ScalarType st) {
  return st == ScalarType::Int8 || st == ScalarType::Int16 ||
         st == ScalarType::Int32 || st == ScalarType::Int64 ||
         is_floating_dtype(st) || is_complex_dtype(st);
}

static ScalarType compat_real_dtype_for_(const Series& s) {
  return s.platform() == Platform::MPS ? ScalarType::Float32 : ScalarType::Float64;
}

static ScalarType special_real_dtype_for_(const Series& s) {
  if (s.platform() == Platform::MPS) return ScalarType::Float32;
  return is_floating_dtype(s.scalar_type()) ? s.scalar_type() : ScalarType::Float64;
}

static double read_real_(const Series& s, std::int64_t i) {
  if (s.scalar_type() == ScalarType::Float16) {
    return static_cast<double>(float16_to_float(reinterpret_cast<const Float16*>(s.raw_data())[i]));
  }
  if (s.scalar_type() == ScalarType::BFloat16) {
    return static_cast<double>(bfloat16_to_float(reinterpret_cast<const BFloat16*>(s.raw_data())[i]));
  }
  if (s.scalar_type() == ScalarType::Float32) {
    return static_cast<double>(reinterpret_cast<const float*>(s.raw_data())[i]);
  }
  return reinterpret_cast<const double*>(s.raw_data())[i];
}

static void write_real_(Series& s, std::int64_t i, double v) {
  if (s.scalar_type() == ScalarType::Float16) {
    reinterpret_cast<Float16*>(s.raw_mutable_data())[i] = float16_from_float(static_cast<float>(v));
    return;
  }
  if (s.scalar_type() == ScalarType::BFloat16) {
    reinterpret_cast<BFloat16*>(s.raw_mutable_data())[i] = bfloat16_from_float(static_cast<float>(v));
    return;
  }
  if (s.scalar_type() == ScalarType::Float32) {
    reinterpret_cast<float*>(s.raw_mutable_data())[i] = static_cast<float>(v);
    return;
  }
  reinterpret_cast<double*>(s.raw_mutable_data())[i] = v;
}

static Series combine_beta_alpha(const Series& base, const Series& update, double beta, double alpha) {
  PLUSS_CHECK(base.scalar_type() == update.scalar_type(),
              "fused add: receiver and product must have the same dtype");
  PLUSS_CHECK(base.platform() == update.platform(),
              "fused add: receiver and product must use the same platform");
  const auto broadcast = SeriesIterator::broadcast_shape(base.sizes(), update.sizes());
  PLUSS_CHECK(broadcast.size() == update.sizes().size() &&
                  std::equal(broadcast.begin(), broadcast.end(), update.sizes().begin()),
              "fused add: receiver must be broadcastable to the product shape");
  if (beta == 1.0 && alpha == 1.0) return ops_internal::add(base, update);
  if (beta == 0.0) return alpha == 1.0 ? update : mul_scalar(update, alpha);
  return ops_internal::add(mul_scalar(base, beta), mul_scalar(update, alpha));
}

static void check_mm_contract_(const Series& lhs, const Series& rhs, const char* op) {
  PLUSS_CHECK(lhs.dim() == 2 && rhs.dim() == 2,
              std::string(op) + ": expected two 2D Series");
  PLUSS_CHECK(lhs.sizes()[1] == rhs.sizes()[0],
              std::string(op) + ": inner dimensions must match");
}

static void check_bmm_contract_(const Series& lhs, const Series& rhs, const char* op) {
  PLUSS_CHECK(lhs.dim() == 3 && rhs.dim() == 3,
              std::string(op) + ": expected two 3D Series");
  PLUSS_CHECK(lhs.sizes()[0] == rhs.sizes()[0],
              std::string(op) + ": batch dimensions must match");
  PLUSS_CHECK(lhs.sizes()[2] == rhs.sizes()[1],
              std::string(op) + ": inner dimensions must match");
}

static void check_mv_contract_(const Series& mat, const Series& vec, const char* op) {
  PLUSS_CHECK(mat.dim() == 2 && vec.dim() == 1,
              std::string(op) + ": expected a 2D matrix and a 1D vector");
  PLUSS_CHECK(mat.sizes()[1] == vec.sizes()[0],
              std::string(op) + ": inner dimensions must match");
}

static void check_outer_contract_(const Series& lhs, const Series& rhs, const char* op) {
  PLUSS_CHECK(lhs.dim() == 1 && rhs.dim() == 1,
              std::string(op) + ": expected two 1D Series");
}

static std::int64_t wrap_dim(std::int64_t dim, std::int64_t nd, const char* op_name) {
  if (dim < 0) dim += nd;
  PLUSS_CHECK(dim >= 0 && dim < nd, std::string(op_name) + ": dim out of range");
  return dim;
}

static Series select_range(const Series& self, std::int64_t dim, std::int64_t start, std::int64_t len) {
  const auto nd = self.dim();
  dim = wrap_dim(dim, nd, "split/chunk/tensor_split");
  const auto dsz = self.sizes()[(std::size_t)dim];
  PLUSS_CHECK(start >= 0 && start <= dsz, "split/chunk/tensor_split: start out of range");
  PLUSS_CHECK(len >= 0, "split/chunk/tensor_split: len must be >= 0");
  if (len == 0) {
    return index_select_dispatch_(
        self,
        dim,
        i64_vector_on(std::span<const std::int64_t>{}, self.platform()));
  }
  std::vector<std::int64_t> idx((std::size_t)len);
  for (std::int64_t i = 0; i < len; ++i) idx[(std::size_t)i] = start + i;
  Series index = i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), self.platform());
  return index_select_dispatch_(self, dim, index);
}

static Series slice_view_impl(
    const Series& self,
    std::int64_t dim,
    std::int64_t start,
    std::int64_t end,
    std::int64_t step) {
  PLUSS_CHECK(self.defined(), "slice: undefined Series");
  const std::int64_t nd = self.dim();
  const std::int64_t d = wrap_dim(dim, nd, "slice");
  PLUSS_CHECK(step != 0, "slice: step must be non-zero");
  PLUSS_CHECK(step > 0, "slice: negative-step slices are unsupported");

  const std::int64_t n = self.sizes()[static_cast<std::size_t>(d)];
  auto normalize_bound = [n](std::int64_t v, bool is_end) -> std::int64_t {
    if (v == std::numeric_limits<std::int64_t>::max()) return is_end ? n : 0;
    if (v < 0) v += n;
    if (v < 0) return 0;
    if (v > n) return n;
    return v;
  };

  const std::int64_t s = normalize_bound(start, false);
  const std::int64_t e = normalize_bound(end, true);
  const std::int64_t len = (e <= s) ? 0 : (1 + (e - 1 - s) / step);

  std::vector<std::int64_t> out_sizes(self.sizes().begin(), self.sizes().end());
  std::vector<std::int64_t> out_strides(self.strides().begin(), self.strides().end());
  const std::size_t ud = static_cast<std::size_t>(d);
  const std::int64_t base_stride = out_strides[ud];
  out_sizes[ud] = len;
  out_strides[ud] = base_stride * step;

  const std::int64_t base_off = self.storage_offset() / std::max<std::int64_t>(1, self.itemsize());
  return as_strided_impl(
      self,
      std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()),
      std::span<const std::int64_t>(out_strides.data(), out_strides.size()),
      base_off + s * base_stride);
}

static Series block_2d_impl(
    const Series& self,
    std::int64_t row_start,
    std::int64_t row_count,
    std::int64_t col_start,
    std::int64_t col_count) {
  PLUSS_CHECK(self.dim() == 2, "block: expected a 2D Series");
  PLUSS_CHECK(row_count >= 0, "block: row_count must be >= 0");
  PLUSS_CHECK(col_count >= 0, "block: col_count must be >= 0");

  const auto rows = self.sizes()[0];
  const auto cols = self.sizes()[1];
  if (row_start < 0) row_start += rows;
  if (col_start < 0) col_start += cols;

  PLUSS_CHECK(row_start >= 0 && row_start <= rows, "block: row_start out of range");
  PLUSS_CHECK(col_start >= 0 && col_start <= cols, "block: col_start out of range");
  PLUSS_CHECK(row_start + row_count <= rows, "block: requested rows exceed dim-0 bounds");
  PLUSS_CHECK(col_start + col_count <= cols, "block: requested cols exceed dim-1 bounds");

  // Delegate through the native slice/view surface so block() inherits the same
  // aliasing and reverse-mode wiring as ordinary basic slicing.
  return self.slice(0, row_start, row_start + row_count)
      .slice(1, col_start, col_start + col_count);
}

static Series split_first_by_size(const Series& self, std::int64_t split_size, std::int64_t dim) {
  PLUSS_CHECK(split_size > 0, "split: split_size must be > 0");
  dim = wrap_dim(dim, self.dim(), "split");
  const auto dsz = self.sizes()[(std::size_t)dim];
  return select_range(self, dim, 0, std::min<std::int64_t>(split_size, dsz));
}

static Series split_first_by_boundaries(const Series& self, std::span<const std::int64_t> boundaries, std::int64_t dim, const char* op_name) {
  dim = wrap_dim(dim, self.dim(), op_name);
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (boundaries.empty()) return self.clone();
  std::int64_t first = boundaries[0];
  if (first < 0) first += dsz;
  first = std::clamp<std::int64_t>(first, 0, dsz);
  return select_range(self, dim, 0, first);
}

static Series chunk_first(const Series& self, std::int64_t chunks, std::int64_t dim) {
  PLUSS_CHECK(chunks > 0, "chunk: chunks must be > 0");
  dim = wrap_dim(dim, self.dim(), "chunk");
  const auto dsz = self.sizes()[(std::size_t)dim];
  const auto split_size = (dsz + chunks - 1) / chunks;
  return select_range(self, dim, 0, std::min<std::int64_t>(split_size, dsz));
}

static Series tensor_split_first(const Series& self, const Arg& sections_or_indices, std::int64_t dim) {
  if (is_numeric_scalar_arg(sections_or_indices)) {
    const auto sections = as_i64(sections_or_indices);
    PLUSS_CHECK(sections > 0, "tensor_split: sections must be > 0");
    return chunk_first(self, sections, dim);
  }
  if (std::holds_alternative<Arg::IntList>(sections_or_indices.v)) {
    return split_first_by_boundaries(self, as_intlist(sections_or_indices), dim, "tensor_split");
  }
  PLUSS_CHECK(false, "tensor_split: expected sections(int) or indices(int[])");
  return Series::undefined();
}

static std::int64_t split_family_dim(const Series& self, std::string_view op_name) {
  if (op_name == "vsplit") {
    PLUSS_CHECK(self.dim() >= 2, "vsplit: expected input with rank >= 2");
    return 0;
  }
  if (op_name == "hsplit") {
    PLUSS_CHECK(self.dim() >= 1, "hsplit: expected input with rank >= 1");
    return self.dim() == 1 ? 0 : 1;
  }
  if (op_name == "dsplit") {
    PLUSS_CHECK(self.dim() >= 3, "dsplit: expected input with rank >= 3");
    return 2;
  }
  PLUSS_CHECK(false, "internal error: unsupported split family op");
  return 0;
}

static Series split_family_first(const Series& self, std::string_view op_name, const Arg& sections_or_indices) {
  const auto dim = split_family_dim(self, op_name);
  return tensor_split_first(self, sections_or_indices, dim);
}

static Series split_first_with_sizes(const Series& self, std::span<const std::int64_t> split_sizes, std::int64_t dim, const char* op_name) {
  PLUSS_CHECK(!split_sizes.empty(), std::string(op_name) + ": expected non-empty split sizes");
  for (const auto v : split_sizes) {
    PLUSS_CHECK(v >= 0, std::string(op_name) + ": split sizes must be >= 0");
  }
  dim = wrap_dim(dim, self.dim(), op_name);
  const auto dsz = self.sizes()[(std::size_t)dim];
  return select_range(self, dim, 0, std::min<std::int64_t>(split_sizes[0], dsz));
}

static Series flip_impl(const Series& self, std::span<const std::int64_t> dims, const char* op_name) {
  PLUSS_CHECK(!dims.empty(), std::string(op_name) + ": expected at least one dimension");
  Series out = self;
  std::vector<std::int64_t> normalized;
  normalized.reserve(dims.size());
  for (const auto d : dims) {
    const auto wd = wrap_dim(d, out.dim(), op_name);
    if (std::find(normalized.begin(), normalized.end(), wd) == normalized.end()) {
      normalized.push_back(wd);
    }
  }
  for (const auto dim : normalized) {
    const auto n = out.sizes()[(std::size_t)dim];
    std::vector<std::int64_t> idx((std::size_t)n);
    for (std::int64_t i = 0; i < n; ++i) {
      idx[(std::size_t)i] = n - 1 - i;
    }
    out = index_select_dispatch_(
        out,
        dim,
        i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), out.platform()));
  }
  return out;
}

static Series diff_impl(const Series& self, std::int64_t n, std::int64_t dim) {
  PLUSS_CHECK(n >= 0, "diff: expected n >= 0");
  if (n == 0) return self.clone();
  dim = wrap_dim(dim, self.dim(), "diff");
  Series out = self;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto dsz = out.sizes()[(std::size_t)dim];
    if (dsz <= 1) {
      return select_range(out, dim, 0, 0);
    }
    const Series hi = select_range(out, dim, 1, dsz - 1);
    const Series lo = select_range(out, dim, 0, dsz - 1);
    out = ops_internal::sub(hi, lo);
  }
  return out;
}

static Series repeat_impl(const Series& self, std::span<const std::int64_t> repeats) {
  PLUSS_CHECK(!repeats.empty(), "repeat: expected non-empty repeats");
  for (const auto r : repeats) PLUSS_CHECK(r >= 0, "repeat: repeats must be >= 0");

  std::vector<std::int64_t> base_sizes(self.sizes().begin(), self.sizes().end());
  std::vector<std::int64_t> reps(repeats.begin(), repeats.end());
  Series out = self;

  if (reps.size() < base_sizes.size()) {
    reps.insert(reps.begin(), base_sizes.size() - reps.size(), 1);
  } else if (reps.size() > base_sizes.size()) {
    std::vector<std::int64_t> new_sizes(reps.size() - base_sizes.size(), 1);
    new_sizes.insert(new_sizes.end(), base_sizes.begin(), base_sizes.end());
    out = self.reshape(std::span<const std::int64_t>(new_sizes.data(), new_sizes.size()));
    base_sizes = std::move(new_sizes);
  }

  for (std::size_t d = 0; d < reps.size(); ++d) {
    const auto rep = reps[d];
    const auto sz = out.sizes()[d];
    if (rep == 0 || sz == 0) {
      return index_select_dispatch_(
          out,
          static_cast<std::int64_t>(d),
          i64_vector_on(std::span<const std::int64_t>{}, out.platform()));
    }
    if (rep == 1) continue;
    std::vector<std::int64_t> idx((std::size_t)(sz * rep));
    for (std::int64_t i = 0; i < sz * rep; ++i) idx[(std::size_t)i] = i % sz;
    out = index_select_dispatch_(
        out,
        static_cast<std::int64_t>(d),
        i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), out.platform()));
  }
  return out;
}

static std::vector<std::int64_t> repeat_factors_like_impl_(const Series& self,
                                                            const Series& like) {
  const std::vector<std::int64_t> src_sizes(self.sizes().begin(), self.sizes().end());
  const std::vector<std::int64_t> dst_sizes(like.sizes().begin(), like.sizes().end());
  if (dst_sizes.empty()) {
    PLUSS_CHECK(src_sizes.empty(),
                "repeat_like: cannot repeat a non-scalar Series to a scalar target");
    return {};
  }
  PLUSS_CHECK(src_sizes.size() <= dst_sizes.size(),
              "repeat_like: self cannot have more dimensions than like");

  std::vector<std::int64_t> aligned_src(dst_sizes.size(), 1);
  if (!src_sizes.empty()) {
    std::copy(src_sizes.begin(),
              src_sizes.end(),
              aligned_src.end() - static_cast<std::ptrdiff_t>(src_sizes.size()));
  }

  std::vector<std::int64_t> repeats(dst_sizes.size(), 1);
  for (std::size_t i = 0; i < dst_sizes.size(); ++i) {
    const std::int64_t src = aligned_src[i];
    const std::int64_t dst = dst_sizes[i];
    PLUSS_CHECK(dst >= 0, "repeat_like: like sizes must be >= 0");
    if (src == 0) {
      PLUSS_CHECK(dst == 0,
                  "repeat_like: zero-sized self dimension can only map to zero-sized like dimension");
      repeats[i] = 1;
      continue;
    }
    PLUSS_CHECK(dst % src == 0,
                "repeat_like: each like dimension must be an integer multiple of self");
    repeats[i] = dst / src;
  }
  return repeats;
}

static Series expand_impl(const Series& self, std::span<const std::int64_t> target_sizes) {
  PLUSS_CHECK(!target_sizes.empty() || self.dim() == 0, "expand: invalid target shape");

  std::vector<std::int64_t> target(target_sizes.begin(), target_sizes.end());
  std::vector<std::int64_t> base_sizes(self.sizes().begin(), self.sizes().end());
  Series out = self;

  if (base_sizes.size() > target.size()) {
    PLUSS_CHECK(false, "expand: cannot expand to fewer dimensions");
  }
  if (base_sizes.size() < target.size()) {
    std::vector<std::int64_t> padded(target.size() - base_sizes.size(), 1);
    padded.insert(padded.end(), base_sizes.begin(), base_sizes.end());
    out = self.reshape(std::span<const std::int64_t>(padded.data(), padded.size()));
    base_sizes = std::move(padded);
  }

  if (out.platform() == Platform::CPU) {
    std::vector<std::int64_t> out_sizes(out.sizes().begin(), out.sizes().end());
    std::vector<std::int64_t> out_strides(out.strides().begin(), out.strides().end());
    bool changed = false;
    for (std::size_t d = 0; d < target.size(); ++d) {
      const auto src = out.sizes()[d];
      auto dst = target[d];
      if (dst == -1) dst = src;
      PLUSS_CHECK(dst >= 0, "expand: target size must be >= 0 or -1");
      if (src == dst) {
        out_sizes[d] = dst;
        continue;
      }
      PLUSS_CHECK(src == 1, "expand: non-singleton dimension mismatch");
      out_sizes[d] = dst;
      out_strides[d] = 0;
      changed = true;
    }
    if (!changed) return out;
    const auto base_off = out.storage_offset() / std::max<std::int64_t>(1, out.itemsize());
    Series view = Series::from_storage(out.storage(), base_off, out_sizes, out_strides, out.scalar_type());
    // The stride-0 view aliases storage directly, so bind the backward here:
    // sum the upstream gradient back over the broadcast axes.
    if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "expand";
      link->parents = {self};
      link->propagate = [self](const Series& g) {
        series_tape::accumulate_sensitivity(self, reduce_broadcasted_like_(g, self, "expand"));
      };
      series_tape::bind_source_link(view, std::move(link));
    }
    return view;
  }

  // Non-CPU fallback: materialize by gather/select.
  for (std::size_t d = 0; d < target.size(); ++d) {
    const auto src = out.sizes()[d];
    auto dst = target[d];
    if (dst == -1) dst = src;
    PLUSS_CHECK(dst >= 0, "expand: target size must be >= 0 or -1");
    if (src == dst) continue;
    PLUSS_CHECK(src == 1, "expand: non-singleton dimension mismatch");
    std::vector<std::int64_t> idx((std::size_t)dst, 0);
    out = index_select_dispatch_(
        out,
        static_cast<std::int64_t>(d),
        i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), out.platform()));
  }
  return out;
}

static Series repeat_interleave_impl(const Series& self, std::int64_t repeats, std::int64_t dim) {
  PLUSS_CHECK(repeats >= 0, "repeat_interleave: repeats must be >= 0");
  dim = wrap_dim(dim, self.dim(), "repeat_interleave");
  const auto sz = self.sizes()[(std::size_t)dim];
  if (repeats == 0 || sz == 0) {
    return index_select_dispatch_(
        self,
        dim,
        i64_vector_on(std::span<const std::int64_t>{}, self.platform()));
  }
  std::vector<std::int64_t> idx((std::size_t)(sz * repeats));
  for (std::int64_t i = 0; i < sz; ++i) {
    for (std::int64_t r = 0; r < repeats; ++r) idx[(std::size_t)(i * repeats + r)] = i;
  }
  return index_select_dispatch_(
      self,
      dim,
      i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), self.platform()));
}

static Series roll_impl(const Series& self, std::span<const std::int64_t> shifts, std::span<const std::int64_t> dims) {
  PLUSS_CHECK(!shifts.empty(), "roll: shifts must be non-empty");
  PLUSS_CHECK(!dims.empty(), "roll: dims must be non-empty");
  PLUSS_CHECK(shifts.size() == dims.size() || shifts.size() == 1,
              "roll: shifts and dims must have same length, or shifts length 1");
  Series out = self;
  for (std::size_t i = 0; i < dims.size(); ++i) {
    const auto dim = wrap_dim(dims[i], out.dim(), "roll");
    const auto n = out.sizes()[(std::size_t)dim];
    if (n <= 1) continue;
    std::int64_t shift = shifts[shifts.size() == 1 ? 0 : i] % n;
    if (shift < 0) shift += n;
    if (shift == 0) continue;
    std::vector<std::int64_t> idx((std::size_t)n);
    for (std::int64_t j = 0; j < n; ++j) idx[(std::size_t)j] = (j - shift + n) % n;
    out = index_select_dispatch_(
        out,
        dim,
        i64_vector_on(std::span<const std::int64_t>(idx.data(), idx.size()), out.platform()));
  }
  return out;
}

static Series roll_flat_impl(const Series& self, std::int64_t shift) {
  auto flat = self.flatten(0, -1);
  const std::array<std::int64_t, 1> shifts{{shift}};
  const std::array<std::int64_t, 1> dims{{0}};
  return roll_impl(flat,
                   std::span<const std::int64_t>(shifts.data(), shifts.size()),
                   std::span<const std::int64_t>(dims.data(), dims.size()))
      .reshape(self.sizes());
}

static void write_scalar_into_ptr(std::byte* dst, ScalarType st, double value) {
  switch (st) {
    case ScalarType::Bool: *reinterpret_cast<std::uint8_t*>(dst) = value != 0.0 ? 1 : 0; return;
    case ScalarType::UInt8: *reinterpret_cast<std::uint8_t*>(dst) = static_cast<std::uint8_t>(value); return;
    case ScalarType::UInt16: *reinterpret_cast<std::uint16_t*>(dst) = static_cast<std::uint16_t>(value); return;
    case ScalarType::UInt32: *reinterpret_cast<std::uint32_t*>(dst) = static_cast<std::uint32_t>(value); return;
    case ScalarType::UInt64: *reinterpret_cast<std::uint64_t*>(dst) = static_cast<std::uint64_t>(value); return;
    case ScalarType::Int8: *reinterpret_cast<std::int8_t*>(dst) = static_cast<std::int8_t>(value); return;
    case ScalarType::Int16: *reinterpret_cast<std::int16_t*>(dst) = static_cast<std::int16_t>(value); return;
    case ScalarType::Int32: *reinterpret_cast<std::int32_t*>(dst) = static_cast<std::int32_t>(value); return;
    case ScalarType::Int64: *reinterpret_cast<std::int64_t*>(dst) = static_cast<std::int64_t>(value); return;
    case ScalarType::Float16: *reinterpret_cast<Float16*>(dst) = float16_from_float(static_cast<float>(value)); return;
    case ScalarType::BFloat16: *reinterpret_cast<BFloat16*>(dst) = bfloat16_from_float(static_cast<float>(value)); return;
    case ScalarType::Float32: *reinterpret_cast<float*>(dst) = static_cast<float>(value); return;
    case ScalarType::Float64: *reinterpret_cast<double*>(dst) = value; return;
    case ScalarType::Complex64: *reinterpret_cast<Complex64*>(dst) = Complex64(static_cast<float>(value), 0.0f); return;
    case ScalarType::Complex128: *reinterpret_cast<Complex128*>(dst) = Complex128(value, 0.0); return;
    default: break;
  }
  PLUSS_CHECK(false, "fill_diagonal_: dtype not implemented");
}

static Series& fill_diagonal_impl(Series& self, double value, bool /*wrap*/) {
  PLUSS_CHECK(self.dim() == 2, "fill_diagonal_: expected a 2D tensor");
  auto x = self.contiguous();
  const auto rows = x.sizes()[0];
  const auto cols = x.sizes()[1];
  const auto n = std::min(rows, cols);
  auto* base = reinterpret_cast<std::byte*>(x.raw_mutable_data());
  const auto item = x.dtype().itemsize;
  for (std::int64_t i = 0; i < n; ++i) {
    const auto off = (i * cols + i) * item;
    write_scalar_into_ptr(base + off, x.scalar_type(), value);
  }
  self = x;
  return self;
}

[[noreturn]] static void throw_cuda_stochastic_unavailable_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    throw_error(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    throw_error(
        std::string(op) +
        ": CUDA native stochastic route declared but kernel is not implemented yet");
  }
  throw_error(
      std::string(op) +
      ": CUDA stochastic path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or enable PLUSS_CUDA_ALLOW_CPU_BRIDGE=1.");
}

template <class CpuRouteFn>
Series cuda_stochastic_route_or_bridge_(
    std::string_view op,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    if (native::cuda::cpu_bridge_enabled()) {
      return cpu_route();
    }
    throw_cuda_stochastic_unavailable_(op, native_enabled, native_strict, route_supported);
  }
  if (native_strict) {
    throw_error(
        std::string(op) +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native::cuda::cpu_bridge_enabled()) {
    return cpu_route();
  }
  throw_cuda_stochastic_unavailable_(op, native_enabled, native_strict, route_supported);
}

ScalarType stochastic_work_dtype_(Platform platform) noexcept {
  return platform == Platform::MPS ? ScalarType::Float32 : ScalarType::Float64;
}

Series rand_work_(pluss::msharp::RangeView::Int sizes, Platform platform, std::uint64_t seed = 0) {
  if (platform == Platform::MPS && native::mps::supports_uniform_stochastic(stochastic_work_dtype_(platform))) {
    return native::mps::uniform_stochastic(sizes, stochastic_work_dtype_(platform), 0.0, 1.0, seed);
  }
  Series out = Series::rand(sizes, stochastic_work_dtype_(platform), seed);
  return platform == Platform::CPU ? out : out.to(platform);
}

Series randn_work_(pluss::msharp::RangeView::Int sizes, Platform platform, std::uint64_t seed = 0) {
  if (platform == Platform::MPS && native::mps::supports_normal_stochastic(stochastic_work_dtype_(platform))) {
    return native::mps::normal_stochastic(sizes, stochastic_work_dtype_(platform), 0.0, 1.0, seed);
  }
  Series out = Series::randn(sizes, stochastic_work_dtype_(platform), seed);
  return platform == Platform::CPU ? out : out.to(platform);
}

static Series bernoulli_sample_from_probs_cpu_(const Series& probs) {
  PLUSS_CHECK(is_floating_dtype(probs.scalar_type()),
              "bernoulli: probability receiver must have a floating-point dtype");
  auto p = probs.to(Platform::CPU).to(ScalarType::Float64).contiguous();
  const auto* values = reinterpret_cast<const double*>(p.raw_data());
  for (std::int64_t i = 0; i < p.numel(); ++i) {
    PLUSS_CHECK(std::isfinite(values[i]) && values[i] >= 0.0 && values[i] <= 1.0,
                "bernoulli: probabilities must be finite and in [0, 1]");
  }
  auto u = Series::rand(p.sizes(), ScalarType::Float64, /*seed=*/0);
  return ops_internal::lt(u, p).to(probs.scalar_type());
}

static Series bernoulli_sample_from_probs_mps_(const Series& probs) {
  return bernoulli_sample_from_probs_cpu_(probs).to(Platform::MPS);
}

static Series bernoulli_sample_from_probs(const Series& probs) {
  if (probs.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_bernoulli_stochastic(probs);
    return cuda_stochastic_route_or_bridge_(
        "bernoulli",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return bernoulli_sample_from_probs_cpu_(probs).to(Platform::CUDA); });
  }
  if (probs.platform() == Platform::MPS) {
    return bernoulli_sample_from_probs_mps_(probs);
  }
  return bernoulli_sample_from_probs_cpu_(probs);
}

static Series uniform_like_cpu_(const Series& self, double from, double to) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type()),
              "uniform_: receiver must have a floating-point or complex dtype");
  PLUSS_CHECK(std::isfinite(from) && std::isfinite(to) && to >= from,
              "uniform_: expected finite bounds with to >= from");
  if (is_complex_dtype(self.scalar_type())) {
    const ScalarType real_dtype = self.scalar_type() == ScalarType::Complex64
                                      ? ScalarType::Float32 : ScalarType::Float64;
    Series real = Series::rand(self.sizes(), ScalarType::Float64, 0);
    Series imag = Series::rand(self.sizes(), ScalarType::Float64, 0);
    real = add_scalar(mul_scalar(real, to - from), from).to(real_dtype).contiguous();
    imag = add_scalar(mul_scalar(imag, to - from), from).to(real_dtype).contiguous();
    Series out = Series::empty(self.sizes(), self.scalar_type());
    if (self.scalar_type() == ScalarType::Complex64) {
      const auto* rp = reinterpret_cast<const float*>(real.raw_data());
      const auto* ip = reinterpret_cast<const float*>(imag.raw_data());
      auto* op = reinterpret_cast<Complex64*>(out.raw_data());
      for (std::int64_t i = 0; i < out.numel(); ++i) op[i] = Complex64(rp[i], ip[i]);
    } else {
      const auto* rp = reinterpret_cast<const double*>(real.raw_data());
      const auto* ip = reinterpret_cast<const double*>(imag.raw_data());
      auto* op = reinterpret_cast<Complex128*>(out.raw_data());
      for (std::int64_t i = 0; i < out.numel(); ++i) op[i] = Complex128(rp[i], ip[i]);
    }
    return out;
  }
  Series x = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  x = add_scalar(mul_scalar(x, to - from), from);
  return x.to(Platform::CPU).to(self.scalar_type());
}

static Series uniform_like(const Series& self, double from, double to) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type()),
              "uniform_: receiver must have a floating-point or complex dtype");
  PLUSS_CHECK(std::isfinite(from) && std::isfinite(to) && to >= from,
              "uniform_: expected finite bounds with to >= from");
  if (is_complex_dtype(self.scalar_type())) {
    Series out = uniform_like_cpu_(self, from, to);
    return self.platform() == Platform::CPU ? out : out.to(self.platform());
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_uniform_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "uniform_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return uniform_like_cpu_(self, from, to).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
                "uniform_: receiver must have a floating-point or complex dtype");
    PLUSS_CHECK(std::isfinite(from) && std::isfinite(to) && to >= from,
                "uniform_: expected finite bounds with to >= from");
    Series x = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    x = add_scalar(mul_scalar(x, to - from), from);
    return x.to(self.scalar_type());
  }
  return uniform_like_cpu_(self, from, to);
}

static Series normal_like_cpu_(const Series& self, double mean, double std) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type()),
              "normal_: receiver must have a floating-point or complex dtype");
  PLUSS_CHECK(std::isfinite(mean) && std::isfinite(std),
              "normal_: mean and std must be finite");
  PLUSS_CHECK(std >= 0.0, "normal_: expected std >= 0");
  if (is_complex_dtype(self.scalar_type())) {
    const ScalarType real_dtype = self.scalar_type() == ScalarType::Complex64
                                      ? ScalarType::Float32 : ScalarType::Float64;
    const double component_std = std / std::sqrt(2.0);
    Series real = add_scalar(mul_scalar(Series::randn(self.sizes(), ScalarType::Float64, 0),
                                        component_std), mean).to(real_dtype).contiguous();
    Series imag = mul_scalar(Series::randn(self.sizes(), ScalarType::Float64, 0),
                             component_std).to(real_dtype).contiguous();
    Series out = Series::empty(self.sizes(), self.scalar_type());
    if (self.scalar_type() == ScalarType::Complex64) {
      const auto* rp = reinterpret_cast<const float*>(real.raw_data());
      const auto* ip = reinterpret_cast<const float*>(imag.raw_data());
      auto* op = reinterpret_cast<Complex64*>(out.raw_data());
      for (std::int64_t i = 0; i < out.numel(); ++i) op[i] = Complex64(rp[i], ip[i]);
    } else {
      const auto* rp = reinterpret_cast<const double*>(real.raw_data());
      const auto* ip = reinterpret_cast<const double*>(imag.raw_data());
      auto* op = reinterpret_cast<Complex128*>(out.raw_data());
      for (std::int64_t i = 0; i < out.numel(); ++i) op[i] = Complex128(rp[i], ip[i]);
    }
    return out;
  }
  Series x = Series::randn(self.sizes(), ScalarType::Float64, /*seed=*/0);
  x = add_scalar(mul_scalar(x, std), mean);
  return x.to(Platform::CPU).to(self.scalar_type());
}

static Series normal_like(const Series& self, double mean, double std) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()) || is_complex_dtype(self.scalar_type()),
              "normal_: receiver must have a floating-point or complex dtype");
  PLUSS_CHECK(std::isfinite(mean) && std::isfinite(std) && std >= 0.0,
              "normal_: expected finite parameters with std >= 0");
  if (is_complex_dtype(self.scalar_type())) {
    Series out = normal_like_cpu_(self, mean, std);
    return self.platform() == Platform::CPU ? out : out.to(self.platform());
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_normal_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "normal_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return normal_like_cpu_(self, mean, std).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(std >= 0.0, "normal_: expected std >= 0");
    Series x = randn_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    x = add_scalar(mul_scalar(x, std), mean);
    return x.to(self.scalar_type());
  }
  return normal_like_cpu_(self, mean, std);
}

static Series exponential_like_cpu_(const Series& self, double lambda) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "exponential_: receiver must have a floating-point dtype");
  PLUSS_CHECK(lambda > 0.0, "exponential_: expected lambda > 0");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  u = ops_internal::clamp_min(u, 1e-12);
  Series x = mul_scalar(ops_internal::log(u), -1.0 / lambda);
  return x.to(Platform::CPU).to(self.scalar_type());
}

static Series exponential_like(const Series& self, double lambda) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "exponential_: receiver must have a floating-point dtype");
  PLUSS_CHECK(std::isfinite(lambda) && lambda > 0.0,
              "exponential_: expected a finite lambda > 0");
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_exponential_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "exponential_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return exponential_like_cpu_(self, lambda).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(lambda > 0.0, "exponential_: expected lambda > 0");
    Series u = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    u = ops_internal::clamp_min(u, 1e-12);
    Series x = mul_scalar(ops_internal::log(u), -1.0 / lambda);
    return x.to(self.scalar_type());
  }
  return exponential_like_cpu_(self, lambda);
}

static Series geometric_like_cpu_(const Series& self, double p) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "geometric_: receiver must have a floating-point dtype");
  PLUSS_CHECK(p > 0.0 && p < 1.0, "geometric_: expected 0 < p < 1");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series one_minus_u = ops_internal::sub(scalar_like(u, 1.0), u);
  const double denom = std::log(1.0 - p);
  Series x = ops_internal::floor(div_scalar(ops_internal::log(one_minus_u), denom));
  x = add_scalar(x, 1.0);
  return x.to(Platform::CPU).to(self.scalar_type());
}

static Series geometric_like(const Series& self, double p) {
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_geometric_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "geometric_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return geometric_like_cpu_(self, p).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
                "geometric_: receiver must have a floating-point dtype");
    PLUSS_CHECK(p > 0.0 && p < 1.0, "geometric_: expected 0 < p < 1");
    Series u = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    Series one_minus_u = ops_internal::sub(scalar_like(u, 1.0), u);
    const double denom = std::log(1.0 - p);
    Series x = ops_internal::floor(div_scalar(ops_internal::log(one_minus_u), denom));
    x = add_scalar(x, 1.0);
    return x.to(self.scalar_type());
  }
  return geometric_like_cpu_(self, p);
}

static Series cauchy_like_cpu_(const Series& self, double median, double sigma) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "cauchy_: receiver must have a floating-point dtype");
  PLUSS_CHECK(sigma > 0.0, "cauchy_: expected sigma > 0");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series centered = sub_scalar(u, 0.5);
  Series t = ops_internal::tan(mul_scalar(centered, kPi));
  Series x = add_scalar(mul_scalar(t, sigma), median);
  return x.to(Platform::CPU).to(self.scalar_type());
}

static Series cauchy_like(const Series& self, double median, double sigma) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "cauchy_: receiver must have a floating-point dtype");
  PLUSS_CHECK(std::isfinite(median) && std::isfinite(sigma) && sigma > 0.0,
              "cauchy_: expected finite parameters with sigma > 0");
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_cauchy_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "cauchy_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return cauchy_like_cpu_(self, median, sigma).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(sigma > 0.0, "cauchy_: expected sigma > 0");
    Series u = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    Series centered = sub_scalar(u, 0.5);
    Series t = ops_internal::tan(mul_scalar(centered, kPi));
    Series x = add_scalar(mul_scalar(t, sigma), median);
    return x.to(self.scalar_type());
  }
  return cauchy_like_cpu_(self, median, sigma);
}

static Series bernoulli_like_cpu_(const Series& self, double p) {
  PLUSS_CHECK(!is_complex_dtype(self.scalar_type()),
              "bernoulli_: complex receivers are not supported");
  PLUSS_CHECK(std::isfinite(p) && p >= 0.0 && p <= 1.0,
              "bernoulli_: expected a finite probability in [0, 1]");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  return ops_internal::lt(u, scalar_like(u, p)).to(Platform::CPU).to(self.scalar_type());
}

static Series bernoulli_like(const Series& self, double p) {
  PLUSS_CHECK(!is_complex_dtype(self.scalar_type()),
              "bernoulli_: complex receivers are not supported");
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_bernoulli_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "bernoulli_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return bernoulli_like_cpu_(self, p).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(std::isfinite(p) && p >= 0.0 && p <= 1.0,
                "bernoulli_: expected a finite probability in [0, 1]");
    Series u = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    return ops_internal::lt(u, scalar_like(u, p)).to(self.scalar_type());
  }
  return bernoulli_like_cpu_(self, p);
}

static Series hardsigmoid_impl_(const Series& self) {
  return ops_internal::hardsigmoid(self);
}

static Series hardswish_impl_(const Series& self) {
  return ops_internal::hardswish(self);
}

static Series mish_impl_(const Series& self) {
  return ops_internal::mish(self);
}

static Series elu_impl_(const Series& self, double alpha, double scale, double input_scale) {
  return ops_internal::elu(self, alpha, scale, input_scale);
}

static Series celu_impl_(const Series& self, double alpha) {
  PLUSS_CHECK(alpha > 0.0, "celu: expected alpha > 0");
  return ops_internal::celu(self, alpha);
}

static Series selu_impl_(const Series& self) {
  return ops_internal::selu(self);
}

static Series threshold_impl_(const Series& self, double threshold, double value) {
  return ops_internal::where(ops_internal::le(self, scalar_like(self, threshold)),
                             scalar_like(self, value),
                             self);
}

static bool is_dropout_family_(std::string_view op) {
  return op == "dropout" || op == "feature_dropout" ||
      op == "alpha_dropout" || op == "feature_alpha_dropout";
}

static bool is_inplace_dropout_family_(std::string_view op) {
  return op == "dropout_" || op == "feature_dropout_" ||
      op == "alpha_dropout_" || op == "feature_alpha_dropout_";
}

static bool is_feature_dropout_(std::string_view op) {
  return op == "feature_dropout" || op == "feature_alpha_dropout" ||
      op == "feature_dropout_" || op == "feature_alpha_dropout_";
}

static bool is_alpha_dropout_(std::string_view op) {
  return op == "alpha_dropout" || op == "feature_alpha_dropout" ||
      op == "alpha_dropout_" || op == "feature_alpha_dropout_";
}

static Series dropout_noise_like_(const Series& self, bool feature, double keep) {
  if (!feature) {
    return bernoulli_like(self, keep).to(self.scalar_type());
  }

  PLUSS_CHECK(self.dim() >= 2, "feature_dropout: expected input with at least 2 dimensions");
  std::vector<std::int64_t> noise_shape(self.dim(), 1);
  noise_shape[0] = self.sizes()[0];
  noise_shape[1] = self.sizes()[1];
  const Series seed =
      Series::empty(pluss::msharp::RangeView::Int(noise_shape), self.scalar_type(), self.platform());
  Series noise = bernoulli_like(seed, keep).to(self.scalar_type());
  return expand_impl(noise, self.sizes());
}

// Dropout's mask/noise is independent of `self` (same reasoning as rrelu's
// slope above), so it's captured from the forward draw rather than
// recomputed -- there is no higher-order pitfall in doing so. Collapses the
// previous mul(+add for alpha-dropout)+div composite into one node:
// out = self*scale (+shift), backward = g*scale.
static Series dropout_affine_fused_(const Series& self, const Series& scale, const Series& shift) {
  Series out;
  {
    series_tape::NoGradGuard no_grad;
    out = shift.defined() ? ops_internal::add(ops_internal::mul(self, scale), shift)
                           : ops_internal::mul(self, scale);
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "dropout";
    link->parents = {self};
    link->propagate = [self, scale](const Series& g) {
      series_tape::accumulate_sensitivity(self, ops_internal::mul(g, scale));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series dropout_family_impl_(std::string_view op, const Series& self, double p, bool train) {
  PLUSS_CHECK(p >= 0.0 && p <= 1.0, "dropout: expected 0 <= p <= 1");
  if (!train || p == 0.0 || self.numel() == 0) return self;
  if (p == 1.0) {
    return Series::zeros(pluss::msharp::RangeView::Int(self.sizes()), self.scalar_type()).to(self.platform());
  }

  const bool feature = is_feature_dropout_(op);
  const bool alpha = is_alpha_dropout_(op);
  const double keep = 1.0 - p;
  Series noise = dropout_noise_like_(self, feature, keep);
  if (!alpha) {
    return dropout_affine_fused_(self, div_scalar(noise, keep), Series());
  }

  constexpr double kAlphaDropoutAlpha = 1.7580993408473766;
  const double a = 1.0 / std::sqrt((kAlphaDropoutAlpha * kAlphaDropoutAlpha * p + 1.0) * keep);
  const double alpha_a = kAlphaDropoutAlpha * a;
  const Series bias = add_scalar(mul_scalar(add_scalar(noise, -1.0), alpha_a), alpha_a * p);
  const Series scale = mul_scalar(noise, a);
  return dropout_affine_fused_(self, scale, bias);
}

static bool series_any_true_(const Series& x) {
  const Series nz = ops_internal::any(ops_internal::ne(x, scalar_like(x, 0.0)));
  return nz.to(ScalarType::Int64).item<std::int64_t>() != 0;
}

static bool series_has_nonfinite_(const Series& x) {
  const Series bad = ops_internal::logical_not(ops_internal::isfinite(x));
  return series_any_true_(bad);
}

static Series maybe_unscale_grad_(const Series& grad, const Series* grad_scale) {
  if (grad_scale == nullptr || !grad_scale->defined()) return grad;
  return ops_internal::div(grad, *grad_scale);
}

static bool should_skip_amp_step_(const Series* found_inf) {
  if (found_inf == nullptr || !found_inf->defined()) return false;
  return series_any_true_(*found_inf);
}

static double step_from_tensor_(const Series& step) {
  PLUSS_CHECK(step.defined() && step.numel() == 1, "optimizer state_step must be a defined scalar Series");
  return step.to(ScalarType::Float64).item<double>();
}

static void write_scalar_value_(Series& s, double value) {
  PLUSS_CHECK(s.defined() && s.numel() == 1, "Expected defined scalar Series");
  switch (s.scalar_type()) {
    case ScalarType::Bool:
      *reinterpret_cast<std::uint8_t*>(s.raw_mutable_data()) = value != 0.0 ? 1u : 0u;
      return;
    case ScalarType::UInt8:
      *reinterpret_cast<std::uint8_t*>(s.raw_mutable_data()) = static_cast<std::uint8_t>(value);
      return;
    case ScalarType::UInt16:
      *reinterpret_cast<std::uint16_t*>(s.raw_mutable_data()) = static_cast<std::uint16_t>(value);
      return;
    case ScalarType::UInt32:
      *reinterpret_cast<std::uint32_t*>(s.raw_mutable_data()) = static_cast<std::uint32_t>(value);
      return;
    case ScalarType::UInt64:
      *reinterpret_cast<std::uint64_t*>(s.raw_mutable_data()) = static_cast<std::uint64_t>(value);
      return;
    case ScalarType::Int8:
      *reinterpret_cast<std::int8_t*>(s.raw_mutable_data()) = static_cast<std::int8_t>(value);
      return;
    case ScalarType::Int16:
      *reinterpret_cast<std::int16_t*>(s.raw_mutable_data()) = static_cast<std::int16_t>(value);
      return;
    case ScalarType::Int32:
      *reinterpret_cast<std::int32_t*>(s.raw_mutable_data()) = static_cast<std::int32_t>(value);
      return;
    case ScalarType::Int64:
      *reinterpret_cast<std::int64_t*>(s.raw_mutable_data()) = static_cast<std::int64_t>(value);
      return;
    case ScalarType::Float16:
      *reinterpret_cast<Float16*>(s.raw_mutable_data()) = float_to_float16(static_cast<float>(value));
      return;
    case ScalarType::BFloat16:
      *reinterpret_cast<BFloat16*>(s.raw_mutable_data()) = float_to_bfloat16(static_cast<float>(value));
      return;
    case ScalarType::Float32:
      *reinterpret_cast<float*>(s.raw_mutable_data()) = static_cast<float>(value);
      return;
    case ScalarType::Float64:
      *reinterpret_cast<double*>(s.raw_mutable_data()) = value;
      return;
    default:
      s = Series::full(s.sizes(), value, s.scalar_type()).to(s.platform());
      return;
  }
}

// rrelu's per-element random slope (training mode) must be the SAME draw in
// forward and backward -- unlike every fused Track B closure elsewhere,
// which recomputes its intermediate fresh from the input for higher-order
// correctness, `slopes` here is independent noise that never depends on
// `self` at all, so capturing the actual forward draw is both correct and
// required (recomputing would silently sample a *different* mask than the
// one the forward pass actually used).
static Series rrelu_slope_fused_(const Series& self, const Series& slopes) {
  Series out;
  {
    series_tape::NoGradGuard no_grad;
    const Series zero = scalar_like(self, 0.0);
    out = ops_internal::where(ops_internal::gt(self, zero), self, ops_internal::mul(self, slopes));
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "rrelu";
    link->parents = {self};
    link->propagate = [self, slopes](const Series& g) {
      const Series zero = scalar_like(self, 0.0);
      const Series one = scalar_like(self, 1.0);
      const Series grad = ops_internal::mul(g, ops_internal::where(ops_internal::gt(self, zero), one, slopes));
      series_tape::accumulate_sensitivity(self, grad);
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series rrelu_impl_(const Series& self, double lower, double upper, bool training) {
  PLUSS_CHECK(lower >= 0.0 && upper >= lower, "rrelu: expected 0 <= lower <= upper");
  if (!training) {
    return ops_internal::leaky_relu(self, 0.5 * (lower + upper));
  }
  const Series slopes = uniform_like(self, lower, upper).to(self.scalar_type());
  return rrelu_slope_fused_(self, slopes);
}

static Series rrelu_with_noise_impl_(
    const Series& self,
    const Series& noise,
    double lower,
    double upper,
    bool training) {
  if (!training) return ops_internal::leaky_relu(self, 0.5 * (lower + upper));
  return rrelu_slope_fused_(self, noise.to(self.scalar_type()));
}

static Series sort_values_only(const Series& self, std::int64_t dim, bool descending, bool stable);
static Series bind_selection_permutation_backward_(
    Series out,
    const Series& self,
    const Series& indices,
    std::int64_t dim,
    const char* op_name);

// NaN positions are replaced with 0 via a differentiable `where` (not
// `nan_to_num`, whose gradient does not zero out the replaced positions), so
// the backward carries `1` on finite positions and `0` on NaN positions.
static Series nan_zeroed_(const Series& self) {
  return ops_internal::where(ops_internal::isnan(self), scalar_like(self, 0.0), self);
}

// Argless `nan_to_num`: NaN maps to 0, +Inf to the largest finite value of the
// dtype, -Inf to the most negative finite value. The kernel takes literal
// doubles, so for a 32-bit series the bound must be the float limit (a double
// limit would overflow to Inf on the narrowing cast and defeat the point).
static void nan_to_num_default_bounds_(ScalarType st, double& posinf, double& neginf) {
  if (st == ScalarType::Float32) {
    posinf = static_cast<double>(std::numeric_limits<float>::max());
    neginf = static_cast<double>(std::numeric_limits<float>::lowest());
  } else {
    posinf = std::numeric_limits<double>::max();
    neginf = std::numeric_limits<double>::lowest();
  }
}

static Series nan_sum_impl(const Series& self) {
  return ops_internal::sum(nan_zeroed_(self));
}

static Series nan_sum_impl(const Series& self, std::int64_t dim, bool keepdim) {
  return ops_internal::sum(nan_zeroed_(self), dim, keepdim);
}

static Series nan_mean_impl(const Series& self) {
  const auto dt = compat_real_dtype_for_(self);
  Series s = ops_internal::sum(nan_zeroed_(self).to(dt));
  Series c = ops_internal::sum(ops_internal::logical_not(ops_internal::isnan(self)).to(dt));
  return ops_internal::div(s, c);
}

static Series nan_mean_impl(const Series& self, std::int64_t dim, bool keepdim) {
  const auto dt = compat_real_dtype_for_(self);
  Series s = ops_internal::sum(nan_zeroed_(self).to(dt), dim, keepdim);
  Series c = ops_internal::sum(ops_internal::logical_not(ops_internal::isnan(self)).to(dt), dim, keepdim);
  return ops_internal::div(s, c);
}

static Series nan_var_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, bool keepdim = false) {
  const auto dt = compat_real_dtype_for_(self);
  const Series x = self.to(dt);
  const Series missing = ops_internal::isnan(x);
  const Series observed = ops_internal::logical_not(missing).to(dt);
  const Series clean = ops_internal::where(missing, scalar_like(x, 0.0), x);
  if (!dim.has_value()) {
    const Series count = ops_internal::sum(observed);
    const Series mean = ops_internal::div(ops_internal::sum(clean), count);
    const Series centered = ops_internal::where(
        missing, scalar_like(x, 0.0), ops_internal::sub(x, mean));
    return ops_internal::div(ops_internal::sum(ops_internal::square(centered)), count);
  }

  const auto d = wrap_dim(*dim, x.dim(), "nanvar");
  const Series count = ops_internal::sum(observed, d, /*keepdim=*/true);
  const Series mean = ops_internal::div(
      ops_internal::sum(clean, d, /*keepdim=*/true), count);
  const Series centered = ops_internal::where(
      missing, scalar_like(x, 0.0), ops_internal::sub(x, mean));
  return ops_internal::div(
      ops_internal::sum(ops_internal::square(centered), d, keepdim),
      keepdim ? count : count.squeeze(d));
}

static Series nan_std_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, bool keepdim = false) {
  return ops_internal::sqrt(nan_var_impl(self, dim, keepdim));
}

template <class Fn>
static Series unary_f64_impl(const Series& self, Fn&& fn) {
  const auto dt = compat_real_dtype_for_(self);
  auto x = self.to(dt).contiguous();
  auto out = Series::empty(x.sizes(), dt);
  const auto n = x.numel();
  for (std::int64_t i = 0; i < n; ++i) {
    write_real_(out, i, fn(read_real_(x, i)));
  }
  return out;
}

template <class Fn>
static Series unary_special_impl_(const Series& self, Fn&& fn) {
  const auto dt = special_real_dtype_for_(self);
  auto x = self.to(dt).contiguous();
  auto out = Series::empty(x.sizes(), dt, x.platform());
  for (std::int64_t i = 0; i < out.numel(); ++i) {
    write_real_(out, i, fn(read_real_(x, i)));
  }
  return out;
}

static double erfinv_approx(double x) {
  if (std::isnan(x)) return x;
  const double ax = std::abs(x);
  if (ax > 1.0) return std::numeric_limits<double>::quiet_NaN();
  if (ax == 1.0) return std::copysign(std::numeric_limits<double>::infinity(), x);

  // Rational initial approximations on the center and tails, followed by the
  // same two Newton corrections used by the established tensor implementation.
  double r;
  if (ax <= 0.7) {
    const double z = x * x;
    const double num = ((-0.140543331 * z + 0.914624893) * z - 1.645349621) * z + 0.886226899;
    const double den = (((0.012229801 * z - 0.329097515) * z + 1.442710462) * z - 2.118377725) * z + 1.0;
    r = x * num / den;
  } else {
    const double z = std::sqrt(-std::log((1.0 - ax) / 2.0));
    const double num = ((1.641345311 * z + 3.429567803) * z - 1.624906493) * z - 1.970840454;
    const double den = (1.637067800 * z + 3.543889200) * z + 1.0;
    r = std::copysign(num / den, x);
  }
  constexpr double two_over_sqrt_pi = 1.1283791670955126;
  for (int i = 0; i < 2; ++i) {
    const double err = std::erf(r) - x;
    r -= err / (two_over_sqrt_pi * std::exp(-r * r));
  }
  return r;
}

static double erfcinv_approx(double y) {
  if (std::isnan(y)) return y;
  if (y < 0.0 || y > 2.0) return std::numeric_limits<double>::quiet_NaN();
  if (y == 0.0) return std::numeric_limits<double>::infinity();
  if (y == 2.0) return -std::numeric_limits<double>::infinity();
  return erfinv_approx(1.0 - y);
}

static Series erfcinv_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) { return erfcinv_approx(x); });
}

static Series betaln_impl(const Series& a, const Series& b) {
  const auto promoted = promote_types(a.scalar_type(), b.scalar_type());
  const auto dt = (a.platform() == Platform::MPS || b.platform() == Platform::MPS)
                      ? ScalarType::Float32
                      : (is_floating_dtype(promoted) ? promoted : ScalarType::Float64);
  auto aa = a.to(dt);
  auto bb = b.to(dt);
  return ops_internal::sub(
      ops_internal::add(ops_internal::lgamma(aa), ops_internal::lgamma(bb)),
      ops_internal::lgamma(ops_internal::add(aa, bb)));
}

static Series beta_impl(const Series& a, const Series& b) {
  return ops_internal::exp(betaln_impl(a, b));
}

// Hurwitz zeta zeta(x, q) = sum_{k>=0} (q + k)^{-x}, for x > 1 and q > 0.
// Stirling/Euler-Maclaurin acceleration (Cephes `zeta`): a short direct sum
// pushes the argument past ~9, then an asymptotic correction with Bernoulli
// coefficients converges to full double precision in a handful of terms --
// far more accurate and far cheaper than a long naive partial sum.
static double hurwitz_zeta_scalar(double x, double q) {
  if (std::isnan(x) || std::isnan(q)) return std::numeric_limits<double>::quiet_NaN();
  if (x == 1.0) return std::numeric_limits<double>::infinity();
  if (x < 1.0) return std::numeric_limits<double>::quiet_NaN();
  if (q <= 0.0) {
    if (q == std::floor(q)) return std::numeric_limits<double>::infinity();
    if (x != std::floor(x)) return std::numeric_limits<double>::quiet_NaN();
  }

  // A[i] = (2 i + 2)! / B_{2 i + 2}; the asymptotic tail terms are t = a*b/A[i].
  static const double A[] = {
      12.0, -720.0, 30240.0, -1209600.0, 47900160.0,
      -1.8924375803183791606e9, 7.47242496e10, -2.950130727918164224e12,
      1.1646782814350067249e14, -4.5979787224074726105e15,
      1.8152105401943546773e17, -7.1661652561756670113e18};
  constexpr double kMachEps = 1.11022302462515654042e-16;

  double s = std::pow(q, -x);
  double a = q;
  int i = 0;
  double b = 0.0;
  while (i < 9 || a <= 9.0) {
    i += 1;
    a += 1.0;
    b = std::pow(a, -x);
    s += b;
    if (std::fabs(b / s) < kMachEps) return s;
  }

  const double w = a;
  s += b * w / (x - 1.0);
  s -= 0.5 * b;
  a = 1.0;
  double k = 0.0;
  for (int j = 0; j < 12; ++j) {
    a *= x + k;
    b /= w;
    const double t = a * b / A[j];
    s += t;
    if (std::fabs(t / s) < kMachEps) return s;
    k += 1.0;
    a *= x + k;
    b /= w;
    k += 1.0;
  }
  return s;
}

static Series zeta_impl(const Series& s, const Series& q) {
  const auto promoted = promote_types(s.scalar_type(), q.scalar_type());
  const auto dt = (s.platform() == Platform::MPS || q.platform() == Platform::MPS)
                      ? ScalarType::Float32
                      : (is_floating_dtype(promoted) ? promoted : ScalarType::Float64);
  auto ss = s.to(dt);
  auto qq = q.to(dt);
  auto shape = SeriesIterator::broadcast_shape(ss.sizes(), qq.sizes());
  auto sse = expand_impl(ss, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto qqe = expand_impl(qq, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, dt);
  const auto n = out.numel();
  for (std::int64_t i = 0; i < n; ++i) {
    write_real_(out, i, hurwitz_zeta_scalar(read_real_(sse, i), read_real_(qqe, i)));
  }
  return out;
}

static Series airy_impl(const Series& self) {
  constexpr double ai0 = 0.35502805388781723926;
  return unary_f64_impl(self, [](double x) {
    if (x == 0.0) return ai0;
    if (x > 0.0) {
      const double t = 2.0 * std::pow(x, 1.5) / 3.0;
      return 0.5 * std::exp(-t) / std::sqrt(kPi) / std::pow(x, 0.25);
    }
    const double ax = -x;
    const double t = 2.0 * std::pow(ax, 1.5) / 3.0;
    return std::sin(t + kPi / 4.0) / std::sqrt(kPi) / std::pow(ax, 0.25);
  });
}

static double bessel_j_series(double nu, double x) {
  double acc = 0.0;
  double half = x * 0.5;
  for (int m = 0; m < 24; ++m) {
    const double sign = (m % 2 == 0) ? 1.0 : -1.0;
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + nu + 1.0);
    const double term = sign * std::pow(half, 2.0 * m + nu) / den;
    acc += term;
    if (std::abs(term) < 1e-16) break;
  }
  return acc;
}

static double bessel_i_series(double nu, double x) {
  double acc = 0.0;
  double half = x * 0.5;
  for (int m = 0; m < 24; ++m) {
    const double den = std::tgamma(m + 1.0) * std::tgamma(m + nu + 1.0);
    const double term = std::pow(half, 2.0 * m + nu) / den;
    acc += term;
    if (std::abs(term) < 1e-16) break;
  }
  return acc;
}

static Series cast_like_(const Series& value, const Series& like);
static Series pdf_impl(const Series& self, double mean, double std);
static Series logpdf_impl(const Series& self, double mean, double std);

enum class NormalStatKind { Pdf, LogPdf, Cdf, LogCdf };

static void bind_normal_stat_backward_(
    const Series& out,
    const Series& self,
    double mean,
    double std,
    NormalStatKind kind,
    const char* op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name;
  link->parents = {self};
  link->propagate = [self, out, mean, std, kind](const Series& out_sens) {
    const Series x = self.to(out.scalar_type());
    const Series centered = ops_internal::sub(x, scalar_like(x, mean));
    Series factor;
    switch (kind) {
      case NormalStatKind::Pdf:
        factor = mul_scalar(ops_internal::mul(centered, out), -1.0 / (std * std));
        break;
      case NormalStatKind::LogPdf:
        factor = mul_scalar(centered, -1.0 / (std * std));
        break;
      case NormalStatKind::Cdf:
        factor = pdf_impl(x, mean, std);
        break;
      case NormalStatKind::LogCdf: {
        const Series log_density = logpdf_impl(x, mean, std);
        factor = ops_internal::exp(ops_internal::sub(log_density, out));
        break;
      }
    }
    const Series grad = ops_internal::mul(out_sens.to(factor.scalar_type()), factor);
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series bessel_impl(const Series& self, double order, std::string_view kind) {
  const double nu = (std::abs(std::sin(kPi * order)) < 1e-8) ? (order + 1e-6) : order;
  if (kind == "j") {
    return unary_f64_impl(self, [nu](double x) { return bessel_j_series(nu, x); });
  }
  if (kind == "y") {
    return unary_f64_impl(self, [nu](double x) {
      const double s = std::sin(kPi * nu);
      return (bessel_j_series(nu, x) * std::cos(kPi * nu) - bessel_j_series(-nu, x)) / s;
    });
  }
  if (kind == "i") {
    return unary_f64_impl(self, [nu](double x) { return bessel_i_series(nu, x); });
  }
  if (kind == "k") {
    return unary_f64_impl(self, [nu](double x) {
      const double s = std::sin(kPi * nu);
      return (kPi * 0.5) * (bessel_i_series(-nu, x) - bessel_i_series(nu, x)) / s;
    });
  }
  PLUSS_CHECK(false, "bessel: unsupported kind");
  return Series::undefined();
}

static Series pdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "pdf: std must be > 0");
  const double inv = 1.0 / (std * std::sqrt(2.0 * kPi));
  Series out = unary_f64_impl(self, [mean, std, inv](double x) {
    const double z = (x - mean) / std;
    return inv * std::exp(-0.5 * z * z);
  });
  bind_normal_stat_backward_(out, self, mean, std, NormalStatKind::Pdf, "pdf");
  return out;
}

static Series logpdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "logpdf: std must be > 0");
  const double log_norm = -std::log(std) - 0.5 * std::log(2.0 * kPi);
  Series out = unary_f64_impl(self, [mean, std, log_norm](double x) {
    const double z = (x - mean) / std;
    return log_norm - 0.5 * z * z;
  });
  bind_normal_stat_backward_(out, self, mean, std, NormalStatKind::LogPdf, "logpdf");
  return out;
}

static Series cdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "cdf: std must be > 0");
  const double den = std * std::sqrt(2.0);
  Series out = unary_f64_impl(self, [mean, den](double x) {
    return 0.5 * std::erfc(-(x - mean) / den);
  });
  bind_normal_stat_backward_(out, self, mean, std, NormalStatKind::Cdf, "cdf");
  return out;
}

static Series logcdf_impl(const Series& self, double mean, double std) {
  PLUSS_CHECK(std > 0.0, "logcdf: std must be > 0");
  const double sqrt2 = std::sqrt(2.0);
  const double log2pi = std::log(2.0 * kPi);
  Series out = unary_f64_impl(self, [mean, std, sqrt2, log2pi](double x) {
    const double z = (x - mean) / std;
    if (std::isnan(z)) return z;
    if (z < -37.0) {
      const double inv = 1.0 / z;
      const double inv2 = inv * inv;
      const double corr = 1.0 - inv2 + 3.0 * inv2 * inv2 - 15.0 * inv2 * inv2 * inv2;
      return -0.5 * z * z - std::log(-z) - 0.5 * log2pi + std::log(std::max(corr, 1e-12));
    }
    if (z > 8.0) {
      const double tail = 0.5 * std::erfc(z / sqrt2);
      return std::log1p(-tail);
    }
    return std::log(0.5 * std::erfc(-z / sqrt2));
  });
  bind_normal_stat_backward_(out, self, mean, std, NormalStatKind::LogCdf, "logcdf");
  return out;
}

enum class MovKind { Sum, Mean, Max, Min, Var, Std };

static void bind_mov_backward_(
    const Series& out,
    const Series& self,
    std::int64_t window,
    std::int64_t dim,
    MovKind kind) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "mov";
  link->parents = {self};
  link->propagate = [self, window, dim, kind](const Series& out_sens) {
    const Series x = self.to(Platform::CPU).to(ScalarType::Float64).contiguous();
    const Series incoming = out_sens.to(Platform::CPU).to(ScalarType::Float64).contiguous();
    Series grad = Series::zeros(self.sizes(), ScalarType::Float64);
    const auto d = wrap_dim(dim, x.dim(), "mov*");
    const auto n = x.sizes()[static_cast<std::size_t>(d)];
    std::int64_t outer = 1;
    std::int64_t inner = 1;
    for (std::int64_t axis = 0; axis < d; ++axis) outer *= x.sizes()[static_cast<std::size_t>(axis)];
    for (std::int64_t axis = d + 1; axis < x.dim(); ++axis) inner *= x.sizes()[static_cast<std::size_t>(axis)];
    for (std::int64_t o = 0; o < outer; ++o) {
      for (std::int64_t i = 0; i < inner; ++i) {
        for (std::int64_t t = 0; t < n; ++t) {
          const auto start = std::max<std::int64_t>(0, t - window + 1);
          const auto len = t - start + 1;
          const auto dst = (o * n + t) * inner + i;
          const double g = read_real_(incoming, dst);
          double mean = 0.0;
          double extremum = (kind == MovKind::Max)
              ? -std::numeric_limits<double>::infinity()
              : std::numeric_limits<double>::infinity();
          for (std::int64_t k = start; k <= t; ++k) {
            const auto off = (o * n + k) * inner + i;
            const double value = read_real_(x, off);
            mean += value;
            if (kind == MovKind::Max) extremum = std::max(extremum, value);
            if (kind == MovKind::Min) extremum = std::min(extremum, value);
          }
          mean /= static_cast<double>(len);
          double variance = 0.0;
          std::int64_t ties = 0;
          for (std::int64_t k = start; k <= t; ++k) {
            const auto off = (o * n + k) * inner + i;
            const double value = read_real_(x, off);
            const double delta = value - mean;
            variance += delta * delta;
            if ((kind == MovKind::Max || kind == MovKind::Min) && value == extremum) ++ties;
          }
          variance /= static_cast<double>(len);
          const double stddev = std::sqrt(variance);
          for (std::int64_t k = start; k <= t; ++k) {
            const auto off = (o * n + k) * inner + i;
            const double value = read_real_(x, off);
            double contribution = 0.0;
            switch (kind) {
              case MovKind::Sum: contribution = g; break;
              case MovKind::Mean: contribution = g / static_cast<double>(len); break;
              case MovKind::Max:
              case MovKind::Min:
                contribution = value == extremum ? g / static_cast<double>(ties) : 0.0;
                break;
              case MovKind::Var:
                contribution = g * 2.0 * (value - mean) / static_cast<double>(len);
                break;
              case MovKind::Std:
                contribution = stddev > 0.0
                    ? g * (value - mean) / (static_cast<double>(len) * stddev)
                    : 0.0;
                break;
            }
            write_real_(grad, off, read_real_(grad, off) + contribution);
          }
        }
      }
    }
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series mov_impl(const Series& self, std::int64_t window, std::int64_t dim, MovKind kind) {
  PLUSS_CHECK(window > 0, "mov*: window must be > 0");
  const auto dt = compat_real_dtype_for_(self);
  auto x = self.to(dt).contiguous();
  dim = wrap_dim(dim, x.dim(), "mov*");
  auto out = Series::empty(x.sizes(), dt);
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      for (std::int64_t t = 0; t < n; ++t) {
        const auto start = std::max<std::int64_t>(0, t - window + 1);
        const auto len = t - start + 1;
        double acc = 0.0;
        double mn = std::numeric_limits<double>::infinity();
        double mx = -std::numeric_limits<double>::infinity();
        for (std::int64_t k = start; k <= t; ++k) {
          const auto off = (o * n + k) * inner + i;
          const double v = read_real_(x, off);
          acc += v;
          mn = std::min(mn, v);
          mx = std::max(mx, v);
        }
        const auto dst = (o * n + t) * inner + i;
        switch (kind) {
          case MovKind::Sum: write_real_(out, dst, acc); break;
          case MovKind::Mean: write_real_(out, dst, acc / static_cast<double>(len)); break;
          case MovKind::Max: write_real_(out, dst, mx); break;
          case MovKind::Min: write_real_(out, dst, mn); break;
          case MovKind::Var: {
            const double mean = acc / static_cast<double>(len);
            double var = 0.0;
            for (std::int64_t k = start; k <= t; ++k) {
              const auto off = (o * n + k) * inner + i;
              const double dv = read_real_(x, off) - mean;
              var += dv * dv;
            }
            write_real_(out, dst, var / static_cast<double>(len));
            break;
          }
          case MovKind::Std: {
            const double mean = acc / static_cast<double>(len);
            double var = 0.0;
            for (std::int64_t k = start; k <= t; ++k) {
              const auto off = (o * n + k) * inner + i;
              const double dv = read_real_(x, off) - mean;
              var += dv * dv;
            }
            write_real_(out, dst, std::sqrt(var / static_cast<double>(len)));
            break;
          }
        }
      }
    }
  }
  bind_mov_backward_(out, self, window, dim, kind);
  return out;
}

static Series ediff1d_impl(const Series& self) {
  return diff_impl(self.flatten(0, -1).to(compat_real_dtype_for_(self)), 1, 0);
}

static Series centered_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto x = self.to(compat_real_dtype_for_(self));
  if (!dim.has_value()) return ops_internal::sub(x, ops_internal::mean(x));
  const auto d = wrap_dim(*dim, x.dim(), "center");
  return ops_internal::sub(x, ops_internal::mean(x, d, /*keepdim=*/true));
}

static Series standardize_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt, double eps = 1e-12) {
  auto centered = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto denom = ops_internal::sqrt(ops_internal::mean(ops_internal::square(centered)));
    return ops_internal::div(centered, add_scalar(denom, eps));
  }
  const auto d = wrap_dim(*dim, centered.dim(), "standardize");
  auto denom = ops_internal::sqrt(ops_internal::mean(ops_internal::square(centered), d, /*keepdim=*/true));
  return ops_internal::div(centered, add_scalar(denom, eps));
}

static Series skewness_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto c = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto m2 = ops_internal::mean(ops_internal::square(c));
    auto m3 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 3.0)));
    return ops_internal::div(m3, ops_internal::pow(m2, scalar_like(m2, 1.5)));
  } else {
    const auto d = wrap_dim(*dim, c.dim(), "skewness");
    auto m2 = ops_internal::mean(ops_internal::square(c), d, false);
    auto m3 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 3.0)), d, false);
    return ops_internal::div(m3, ops_internal::pow(m2, scalar_like(m2, 1.5)));
  }
}

static Series kurtosis_impl(const Series& self, std::optional<std::int64_t> dim = std::nullopt) {
  auto c = centered_impl(self, dim);
  if (!dim.has_value()) {
    auto m2 = ops_internal::mean(ops_internal::square(c));
    auto m4 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 4.0)));
    return sub_scalar(ops_internal::div(m4, ops_internal::pow(m2, scalar_like(m2, 2.0))), 3.0);
  } else {
    const auto d = wrap_dim(*dim, c.dim(), "kurtosis");
    auto m2 = ops_internal::mean(ops_internal::square(c), d, false);
    auto m4 = ops_internal::mean(ops_internal::pow(c, scalar_like(c, 4.0)), d, false);
    return sub_scalar(ops_internal::div(m4, ops_internal::pow(m2, scalar_like(m2, 2.0))), 3.0);
  }
}

static Series norm_along_impl(const Series& self, std::int64_t dim, double eps = 1e-12) {
  auto x = self.to(compat_real_dtype_for_(self));
  const auto d = wrap_dim(dim, x.dim(), "normr/normc");
  auto denom = ops_internal::sqrt(ops_internal::sum(ops_internal::square(x), d, /*keepdim=*/true)).contiguous();
  if (denom.scalar_type() == ScalarType::Float64) {
    auto* p = reinterpret_cast<double*>(denom.raw_mutable_data());
    for (std::int64_t i = 0; i < denom.numel(); ++i) p[i] += eps;
  } else if (denom.scalar_type() == ScalarType::Float32) {
    auto* p = reinterpret_cast<float*>(denom.raw_mutable_data());
    const float e = static_cast<float>(eps);
    for (std::int64_t i = 0; i < denom.numel(); ++i) p[i] += e;
  } else {
    denom = add_scalar(denom, eps);
  }
  return ops_internal::div(x, denom);
}

static Series cdist_forward_primitives_(const Series& a, const Series& b, double p) {
  const Series diff = ops_internal::sub(a.unsqueeze(1), b.unsqueeze(0));
  const Series ad = ops_internal::abs(diff);
  if (p == 2.0) {
    return ops_internal::sqrt(ops_internal::sum(ops_internal::square(ad), 2, false));
  }
  return ops_internal::pow(
      ops_internal::sum(ops_internal::pow(ad, scalar_like(ad, p)), 2, false),
      scalar_like(ad, 1.0 / p));
}

static void bind_cdist_euclidean_backward_(
    const Series& out,
    const Series& a,
    const Series& b,
    ScalarType dtype) {
  const bool track_a = series_tape::recording_enabled() && series_tape::tracks_slope(a);
  const bool track_b = series_tape::recording_enabled() && series_tape::tracks_slope(b);
  if (!track_a && !track_b) return;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "cdist";
  link->parents = {a, b};
  link->propagate = [a, b, out, dtype](const Series& out_sens) {
    const Series aa = a.to(dtype);
    const Series bb = b.to(dtype);
    const Series diff = ops_internal::sub(aa.unsqueeze(1), bb.unsqueeze(0));
    const Series positive = ops_internal::gt(out, scalar_like(out, 0.0));
    const Series safe_distance = ops_internal::where(
        positive, out, scalar_like(out, 1.0));
    const Series scale = ops_internal::div(out_sens.to(dtype), safe_distance).unsqueeze(2);
    const Series pair_grad = ops_internal::mul(diff, scale);

    if (series_tape::tracks_slope(a)) {
      const Series grad_a = ops_internal::sum(pair_grad, 1, false);
      series_tape::accumulate_sensitivity(a, cast_like_(grad_a, a));
    }
    if (series_tape::tracks_slope(b)) {
      const Series grad_b = ops_internal::neg(ops_internal::sum(pair_grad, 0, false));
      series_tape::accumulate_sensitivity(b, cast_like_(grad_b, b));
    }
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series cdist_impl(const Series& a, const Series& b, double p = 2.0) {
  ops_internal::require_same_platform(a, b, "cdist");
  PLUSS_CHECK(a.dim() == 2 && b.dim() == 2, "cdist/pdist2: expected two 2D tensors");
  PLUSS_CHECK(a.sizes()[1] == b.sizes()[1], "cdist/pdist2: feature dimension mismatch");
  const auto dt = promote_types(compat_real_dtype_for_(a), compat_real_dtype_for_(b));

  const bool track_euclidean = p == 2.0 && series_tape::recording_enabled() &&
      (series_tape::tracks_slope(a) || series_tape::tracks_slope(b));
  if (track_euclidean) {
    Series out;
    {
      series_tape::NoGradGuard no_grad;
      out = cdist_forward_primitives_(a.to(dt), b.to(dt), p);
    }
    bind_cdist_euclidean_backward_(out, a, b, dt);
    return out;
  }

  return cdist_forward_primitives_(a.to(dt), b.to(dt), p);
}

enum class TrigXform { Dct, Idct, Dst, Idst };

static void bind_trig_xform_backward_(
    const Series& out,
    const Series& self,
    std::int64_t dim,
    TrigXform kind) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = kind == TrigXform::Dct ? "dct" :
      kind == TrigXform::Idct ? "idct" :
      kind == TrigXform::Dst ? "dst" : "idst";
  link->parents = {self};
  link->propagate = [self, dim, kind](const Series& out_sens) {
    series_tape::NoGradGuard no_grad;
    const Series incoming = out_sens.to(Platform::CPU).to(ScalarType::Float64).contiguous();
    Series grad = Series::zeros(self.sizes(), ScalarType::Float64, Platform::CPU);
    const auto n = self.sizes()[(std::size_t)dim];
    const auto nd = self.dim();
    std::int64_t outer = 1;
    std::int64_t inner = 1;
    for (std::int64_t d = 0; d < dim; ++d) outer *= self.sizes()[(std::size_t)d];
    for (std::int64_t d = dim + 1; d < nd; ++d) inner *= self.sizes()[(std::size_t)d];
    const auto* source = reinterpret_cast<const double*>(incoming.raw_data());
    auto* destination = reinterpret_cast<double*>(grad.raw_mutable_data());
    const double inv_n = n == 0 ? 0.0 : 1.0 / static_cast<double>(n);
    for (std::int64_t o = 0; o < outer; ++o) {
      for (std::int64_t i = 0; i < inner; ++i) {
        for (std::int64_t t = 0; t < n; ++t) {
          double contribution = 0.0;
          for (std::int64_t k = 0; k < n; ++k) {
            double coefficient = 0.0;
            switch (kind) {
              case TrigXform::Dct:
                coefficient = std::cos(
                    kPi * (static_cast<double>(t) + 0.5) * static_cast<double>(k) * inv_n);
                break;
              case TrigXform::Idct:
                coefficient = t == 0 ? inv_n :
                    2.0 * inv_n * std::cos(
                        kPi * static_cast<double>(t) *
                        (static_cast<double>(k) + 0.5) * inv_n);
                break;
              case TrigXform::Dst:
                coefficient = std::sin(
                    kPi * (static_cast<double>(t) + 0.5) *
                    (static_cast<double>(k) + 1.0) * inv_n);
                break;
              case TrigXform::Idst:
                coefficient = 2.0 * inv_n * std::sin(
                    kPi * (static_cast<double>(t) + 1.0) *
                    (static_cast<double>(k) + 0.5) * inv_n);
                break;
            }
            contribution += source[(o * n + k) * inner + i] * coefficient;
          }
          destination[(o * n + t) * inner + i] = contribution;
        }
      }
    }
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series trig_xform_impl(const Series& self, std::int64_t dim, TrigXform kind) {
  const auto dt = compat_real_dtype_for_(self);
  auto x = self.to(dt).contiguous();
  dim = wrap_dim(dim, x.dim(), "dct/idct/dst/idst");
  auto out = Series::empty(x.sizes(), dt);
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  if (n == 0) {
    bind_trig_xform_backward_(out, self, dim, kind);
    return out;
  }

  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];

  const double inv_n = 1.0 / static_cast<double>(n);
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      for (std::int64_t k = 0; k < n; ++k) {
        double acc = 0.0;
        switch (kind) {
          case TrigXform::Dct:
            for (std::int64_t t = 0; t < n; ++t) {
              const auto off = (o * n + t) * inner + i;
              const double basis = std::cos(kPi * (static_cast<double>(t) + 0.5) * static_cast<double>(k) * inv_n);
              acc += read_real_(x, off) * basis;
            }
            break;
          case TrigXform::Idct: {
            acc += 0.5 * read_real_(x, (o * n + 0) * inner + i);
            for (std::int64_t t = 1; t < n; ++t) {
              const auto off = (o * n + t) * inner + i;
              const double basis = std::cos(kPi * static_cast<double>(t) * (static_cast<double>(k) + 0.5) * inv_n);
              acc += read_real_(x, off) * basis;
            }
            acc *= (2.0 * inv_n);
            break;
          }
          case TrigXform::Dst:
            for (std::int64_t t = 0; t < n; ++t) {
              const auto off = (o * n + t) * inner + i;
              const double basis =
                  std::sin(kPi * (static_cast<double>(t) + 0.5) * (static_cast<double>(k) + 1.0) * inv_n);
              acc += read_real_(x, off) * basis;
            }
            break;
          case TrigXform::Idst:
            for (std::int64_t t = 0; t < n; ++t) {
              const auto off = (o * n + t) * inner + i;
              const double basis =
                  std::sin(kPi * (static_cast<double>(t) + 1.0) * (static_cast<double>(k) + 0.5) * inv_n);
              acc += read_real_(x, off) * basis;
            }
            acc *= (2.0 * inv_n);
            break;
        }
        write_real_(out, (o * n + k) * inner + i, acc);
      }
    }
  }
  bind_trig_xform_backward_(out, self, dim, kind);
  return out;
}

static Series hilbert_impl(const Series& self, std::int64_t dim = -1) {
  if (self.platform() != Platform::MPS) {
    const auto d = wrap_dim(dim, self.dim(), "hilbert");
    Series out = native::cpu::hilbert_transform(self, d);
    if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
      auto link = std::make_shared<series_tape::SourceLink>();
      link->op_name = "hilbert";
      link->parents = {self};
      link->propagate = [self, d](const Series& out_sens) {
        series_tape::NoGradGuard no_grad;
        Series grad = native::cpu::hilbert_transform(
            out_sens.to(Platform::CPU).to(ScalarType::Complex128), d);
        if (!is_complex_dtype(self.scalar_type())) grad = ops_internal::real(grad);
        series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
      };
      series_tape::bind_source_link(out, std::move(link));
    }
    return out;
  }
  // Keep a platform-native fallback until dedicated complex MPS kernels are added.
  (void)dim;
  return self.clone();
}

// --- Short-time Fourier transform -----------------------------------------
// Built entirely from differentiable primitives (pad via slice/flip/cat,
// framing via unfold, windowing via mul, the DFT as a constant-matrix matmul)
// so the reverse pass comes for free and matches libtorch's composite graph.

static Series stft_pad_center_(const Series& x, std::int64_t pad, std::string_view mode) {
  if (pad == 0) return x;
  const std::int64_t last = x.dim() - 1;
  const std::int64_t T = x.sizes()[static_cast<std::size_t>(last)];
  if (mode == "constant") {
    std::vector<std::int64_t> psz(x.sizes().begin(), x.sizes().end());
    psz[static_cast<std::size_t>(last)] = pad;
    const Series z = Series::zeros(psz, x.scalar_type()).to(x.platform());
    return z.unite({x, z}, last);
  }
  PLUSS_CHECK(mode == "reflect",
              "stft: only pad_mode 'reflect' and 'constant' are supported");
  PLUSS_CHECK(pad < T, "stft: reflect padding requires n_fft/2 < signal length");
  const Series left = x.slice(last, 1, pad + 1).flip(last);
  const Series right = x.slice(last, T - pad - 1, T - 1).flip(last);
  return left.unite({x, right}, last);
}

static Series stft_impl(const Series& self, std::int64_t n_fft, std::int64_t hop_length,
                        std::int64_t win_length, const Series& window, bool center,
                        std::string_view pad_mode, bool normalized, bool onesided,
                        bool return_complex) {
  PLUSS_CHECK(self.dim() == 1 || self.dim() == 2, "stft: expected a 1D or 2D tensor");
  PLUSS_CHECK(n_fft > 0, "stft: n_fft must be > 0");
  const bool was_1d = self.dim() == 1;
  const std::int64_t hop = hop_length > 0 ? hop_length : (n_fft >> 2);
  const std::int64_t win = win_length > 0 ? win_length : n_fft;
  PLUSS_CHECK(win > 0 && win <= n_fft, "stft: expected 0 < win_length <= n_fft");

  const ScalarType rdt =
      (self.scalar_type() == ScalarType::Float32) ? ScalarType::Float32 : ScalarType::Float64;
  Series x = self.to(rdt);
  if (was_1d) x = x.unsqueeze(0);
  if (center) x = stft_pad_center_(x, n_fft / 2, pad_mode);

  const std::int64_t batch = x.sizes()[0];
  const std::int64_t len = x.sizes()[1];
  PLUSS_CHECK(n_fft <= len, "stft: expected n_fft <= (padded) signal length");
  const std::int64_t n_frames = 1 + (len - n_fft) / hop;

  Series frames = x.unfold(1, n_fft, hop);  // [batch, n_frames, n_fft]

  if (window.defined() || win < n_fft) {
    Series w = window.defined() ? window.to(rdt).to(x.platform())
                                : Series::ones({win}, rdt).to(x.platform());
    PLUSS_CHECK(w.dim() == 1 && w.sizes()[0] == win,
                "stft: window must be 1D of length win_length");
    if (win < n_fft) {
      const std::int64_t left = (n_fft - win) / 2;
      const Series zl = Series::zeros({left}, rdt).to(x.platform());
      const Series zr = Series::zeros({n_fft - win - left}, rdt).to(x.platform());
      w = zl.unite({w, zr}, 0);
    }
    frames = frames.mul(w);
  }

  const std::int64_t n_freq = onesided ? (n_fft / 2 + 1) : n_fft;
  const Series nrange = Series::arange(0, n_fft, 1, rdt).to(x.platform()).reshape({n_fft, 1});
  const Series krange = Series::arange(0, n_freq, 1, rdt).to(x.platform()).reshape({1, n_freq});
  const Series ang = nrange.mul(krange).mul(2.0 * kPi / static_cast<double>(n_fft));
  const Series cos_mat = ang.cos();       // [n_fft, n_freq]
  const Series nsin_mat = ang.sin().neg();

  Series flat = frames.reshape({batch * n_frames, n_fft});
  Series re = flat.matmul(cos_mat).reshape({batch, n_frames, n_freq});
  Series im = flat.matmul(nsin_mat).reshape({batch, n_frames, n_freq});
  if (normalized) {
    const double s = 1.0 / std::sqrt(static_cast<double>(n_fft));
    re = re.mul(s);
    im = im.mul(s);
  }
  re = re.transpose(1, 2);  // [batch, n_freq, n_frames]
  im = im.transpose(1, 2);
  if (was_1d) {
    re = re.squeeze(0);
    im = im.squeeze(0);
  }
  if (return_complex) {
    const ScalarType cdt =
        (rdt == ScalarType::Float32) ? ScalarType::Complex64 : ScalarType::Complex128;
    return ops_internal::complex_from_parts(re, im, cdt);
  }
  return re.pile({im}, re.dim());  // [..., n_freq, n_frames, 2]
}

// Overlap-add: the adjoint of `unfold`. frames [B, n_frames, n_fft] -> [B, out_len],
// out[b, f*hop + p] += frames[b, f, p]. Built on scatter_add so it is
// differentiable (its own reverse is a gather / unfold).
static Series stft_overlap_add_(const Series& frames, std::int64_t hop, std::int64_t out_len) {
  const std::int64_t batch = frames.sizes()[0];
  const std::int64_t n_frames = frames.sizes()[1];
  const std::int64_t n_fft = frames.sizes()[2];
  Series idx2d = Series::empty({n_frames * n_fft}, ScalarType::Int64).to(frames.platform());
  auto* ip = reinterpret_cast<std::int64_t*>(idx2d.raw_mutable_data());
  for (std::int64_t f = 0; f < n_frames; ++f)
    for (std::int64_t p = 0; p < n_fft; ++p)
      ip[f * n_fft + p] = f * hop + p;
  Series idx_row = idx2d.reshape({1, n_frames * n_fft});
  Series index = batch == 1 ? idx_row : idx_row.repeat(std::vector<std::int64_t>{batch, 1});
  const Series flat = frames.reshape({batch, n_frames * n_fft});
  const Series base = Series::zeros({batch, out_len}, frames.scalar_type()).to(frames.platform());
  return base.scatter_add(1, index, flat);
}

static Series istft_impl(const Series& self, std::int64_t n_fft, std::int64_t hop_length,
                         std::int64_t win_length, const Series& window, bool center,
                         bool normalized, bool onesided, std::int64_t length) {
  const std::int64_t hop = hop_length > 0 ? hop_length : (n_fft >> 2);
  const std::int64_t win = win_length > 0 ? win_length : n_fft;
  PLUSS_CHECK(win > 0 && win <= n_fft && hop > 0 && hop <= win,
              "istft: expected 0 < hop_length <= win_length <= n_fft");
  PLUSS_CHECK(is_complex_dtype(self.scalar_type()),
              "istft requires a complex-valued input (stft(..., return_complex=true))");
  const bool was_2d = self.dim() == 2;
  PLUSS_CHECK(was_2d || self.dim() == 3, "istft: expected a 2D or 3D complex tensor");

  Series spec = was_2d ? self.unsqueeze(0) : self;  // [B, n_freq, n_frames]
  const std::int64_t batch = spec.sizes()[0];
  const std::int64_t n_freq = spec.sizes()[1];
  const std::int64_t n_frames = spec.sizes()[2];
  PLUSS_CHECK(onesided ? (n_freq == n_fft / 2 + 1) : (n_freq == n_fft),
              "istft: frequency dimension does not match n_fft / onesided");

  const ScalarType rdt = (self.scalar_type() == ScalarType::Complex64) ? ScalarType::Float32
                                                                       : ScalarType::Float64;
  Series re = ops_internal::real(spec).transpose(1, 2);  // [B, n_frames, n_freq]
  Series im = ops_internal::imag(spec).transpose(1, 2);

  // inverse DFT as a real matmul with per-bin weights (1 at DC / Nyquist, 2 on
  // the interior bins when onesided) and the 1/N (or 1/sqrt(N)) scaling.
  std::vector<double> wk(static_cast<std::size_t>(n_freq), 1.0);
  if (onesided)
    for (std::int64_t k = 1; k < n_freq - (n_fft % 2 == 0 ? 1 : 0); ++k) wk[static_cast<std::size_t>(k)] = 2.0;
  const double denom = normalized ? std::sqrt(static_cast<double>(n_fft))
                                  : static_cast<double>(n_fft);
  Series Cinv = Series::empty({n_freq, n_fft}, rdt);
  Series Sinv = Series::empty({n_freq, n_fft}, rdt);
  for (std::int64_t k = 0; k < n_freq; ++k) {
    for (std::int64_t nn = 0; nn < n_fft; ++nn) {
      const double a = 2.0 * kPi * static_cast<double>(k) * static_cast<double>(nn) /
                       static_cast<double>(n_fft);
      const double w = wk[static_cast<std::size_t>(k)] / denom;
      write_real_(Cinv, k * n_fft + nn, w * std::cos(a));
      write_real_(Sinv, k * n_fft + nn, -w * std::sin(a));
    }
  }
  Cinv = Cinv.to(spec.platform());
  Sinv = Sinv.to(spec.platform());

  Series re_flat = re.reshape({batch * n_frames, n_freq});
  Series im_flat = im.reshape({batch * n_frames, n_freq});
  Series frames = re_flat.matmul(Cinv).add(im_flat.matmul(Sinv))
                     .reshape({batch, n_frames, n_fft});  // [B, n_frames, n_fft]

  // window (centered/padded to n_fft), applied to the reconstructed frames.
  Series w = window.defined() ? window.to(rdt).to(spec.platform())
                              : Series::ones({win}, rdt).to(spec.platform());
  if (win < n_fft) {
    const std::int64_t left = (n_fft - win) / 2;
    const Series zl = Series::zeros({left}, rdt).to(spec.platform());
    const Series zr = Series::zeros({n_fft - win - left}, rdt).to(spec.platform());
    w = zl.unite({w, zr}, 0);
  }
  frames = frames.mul(w);

  const std::int64_t expected = n_fft + hop * (n_frames - 1);
  Series y = stft_overlap_add_(frames, hop, expected);  // [B, expected]

  const Series w2 = w.mul(w).reshape({1, 1, n_fft}).repeat(std::vector<std::int64_t>{1, n_frames, 1});
  Series envelope = stft_overlap_add_(w2, hop, expected);  // [1, expected]

  const std::int64_t start = center ? n_fft / 2 : 0;
  std::int64_t end = expected;
  if (length > 0) end = start + length;
  else if (center) end = expected - n_fft / 2;
  y = y.slice(1, start, end);
  envelope = envelope.slice(1, start, end);
  y = y.div(envelope);
  if (!was_2d) return y;
  return y.squeeze(0);
}

static Series shift_impl(const Series& self, std::span<const std::int64_t> dims, bool inverse) {
  Series out = self;
  std::vector<std::int64_t> use_dims;
  if (dims.empty()) {
    use_dims.resize((std::size_t)self.dim());
    for (std::int64_t d = 0; d < self.dim(); ++d) use_dims[(std::size_t)d] = d;
  } else {
    use_dims.assign(dims.begin(), dims.end());
  }
  for (auto& d : use_dims) d = wrap_dim(d, out.dim(), "fftshift/ifftshift");
  for (const auto d : use_dims) {
    const auto n = out.sizes()[(std::size_t)d];
    const auto s = inverse ? ((n + 1) / 2) : (n / 2);
    const std::array<std::int64_t, 1> shifts{{s}};
    const std::array<std::int64_t, 1> dd{{d}};
    out = roll_impl(out,
                    std::span<const std::int64_t>(shifts.data(), shifts.size()),
                    std::span<const std::int64_t>(dd.data(), dd.size()));
  }
  return out;
}

static Series unwrap_impl(const Series& self, double discont = kPi, std::int64_t dim = -1) {
  const auto dt = compat_real_dtype_for_(self);
  auto x = self.to(dt).contiguous();
  dim = wrap_dim(dim, x.dim(), "unwrap");
  auto out = Series::empty(x.sizes(), dt);
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      double offset = 0.0;
      const auto off0 = (o * n + 0) * inner + i;
      write_real_(out, off0, read_real_(x, off0));
      for (std::int64_t t = 1; t < n; ++t) {
        const auto prev = (o * n + (t - 1)) * inner + i;
        const auto cur = (o * n + t) * inner + i;
        const double delta = read_real_(x, cur) - read_real_(x, prev);
        if (delta > discont) offset -= 2.0 * kPi;
        else if (delta < -discont) offset += 2.0 * kPi;
        write_real_(out, cur, read_real_(x, cur) + offset);
      }
    }
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "unwrap";
    link->parents = {self};
    link->propagate = [self](const Series& out_sens) {
      series_tape::accumulate_sensitivity(self, cast_like_(out_sens, self));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series cplxpair_impl(const Series& self) {
  if (!is_complex_dtype(self.scalar_type())) return sort_values_only(self, -1, false, false);
  auto x = self.to(ScalarType::Complex128).contiguous();
  auto flat = x.flatten(0, -1).contiguous();
  const auto n = flat.numel();
  std::vector<std::pair<Complex128, std::int64_t>> vals((std::size_t)n);
  const auto* ip = reinterpret_cast<const Complex128*>(flat.raw_data());
  for (std::int64_t i = 0; i < n; ++i) vals[(std::size_t)i] = {ip[i], i};
  std::stable_sort(vals.begin(), vals.end(), [](const auto& a, const auto& b) {
    if (std::real(a.first) == std::real(b.first)) return std::imag(a.first) < std::imag(b.first);
    return std::real(a.first) < std::real(b.first);
  });
  auto out_flat = Series::empty({n}, ScalarType::Complex128);
  auto* op = reinterpret_cast<Complex128*>(out_flat.raw_data());
  std::vector<std::int64_t> order((std::size_t)n);
  for (std::int64_t i = 0; i < n; ++i) {
    op[i] = vals[(std::size_t)i].first;
    order[(std::size_t)i] = vals[(std::size_t)i].second;
  }
  Series out = out_flat.reshape(self.sizes());
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "cplxpair";
    link->parents = {self};
    link->propagate = [self, order = std::move(order)](const Series& out_sens) {
      series_tape::NoGradGuard no_grad;
      const Series incoming = out_sens.to(Platform::CPU).to(ScalarType::Complex128).contiguous();
      const auto* source = reinterpret_cast<const Complex128*>(incoming.raw_data());
      Series grad = Series::zeros(self.sizes(), ScalarType::Complex128, Platform::CPU);
      auto* destination = reinterpret_cast<Complex128*>(grad.raw_mutable_data());
      for (std::size_t i = 0; i < order.size(); ++i) destination[order[i]] = source[i];
      series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series abs2_impl(const Series& self) {
  return ops_internal::square(ops_internal::abs(self));
}

static Series vdot_impl(const Series& self, const Series& other) {
  PLUSS_CHECK(self.dim() == 1 && other.dim() == 1, "vdot: expected two 1D Series");
  PLUSS_CHECK(self.numel() == other.numel(), "vdot: inputs must have the same length");
  return ops_internal::sum(ops_internal::mul(ops_internal::conj(self), other));
}

static Series logit_impl(const Series& self, std::optional<double> eps = std::nullopt) {
  auto x = self.to(ScalarType::Float64);
  if (eps.has_value()) {
    const double e = std::clamp(*eps, 0.0, 0.5);
    x = ops_internal::clamp(x, e, 1.0 - e);
  }
  return ops_internal::log(ops_internal::div(x, ops_internal::sub(scalar_like(x, 1.0), x)));
}

static Series nextafter_impl(const Series& self, const Series& other) {
  auto a = self.to(ScalarType::Float64);
  auto b = other.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const double*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const double*>(bb.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::nextafter(ap[i], bp[i]);
  return out;
}

static Series nextafter_impl(const Series& self, double toward) {
  return nextafter_impl(self, scalar_like(self, toward));
}

static Series random_like_cpu_(const Series& self, double lo, double hi) {
  PLUSS_CHECK(hi > lo, "random_: expected hi > lo");
  PLUSS_CHECK(!is_complex_dtype(self.scalar_type()), "random_: complex receivers are not supported");
  Series u = Series::rand(self.sizes(), ScalarType::Float64, /*seed=*/0);
  Series x = ops_internal::floor(add_scalar(mul_scalar(u, hi - lo), lo));
  return x.to(Platform::CPU).to(self.scalar_type());
}

static void check_random_bounds_fit_(ScalarType dtype, double lo, double hi) {
  auto within = [lo, hi](long double minimum, long double maximum) {
    return static_cast<long double>(lo) >= minimum &&
           static_cast<long double>(hi) - 1.0L <= maximum;
  };
  bool valid = false;
  switch (dtype) {
    case ScalarType::Bool: valid = lo >= 0.0 && hi <= 2.0; break;
    case ScalarType::UInt8: valid = within(0, UINT8_MAX); break;
    case ScalarType::UInt16: valid = within(0, UINT16_MAX); break;
    case ScalarType::UInt32: valid = within(0, UINT32_MAX); break;
    case ScalarType::UInt64: valid = lo >= 0.0; break;
    case ScalarType::Int8: valid = within(INT8_MIN, INT8_MAX); break;
    case ScalarType::Int16: valid = within(INT16_MIN, INT16_MAX); break;
    case ScalarType::Int32: valid = within(INT32_MIN, INT32_MAX); break;
    case ScalarType::Int64: valid = true; break;
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64: valid = true; break;
    default: break;
  }
  PLUSS_CHECK(valid, "random_: requested range is outside the receiver dtype");
}

static Series random_like(const Series& self, double lo, double hi) {
  PLUSS_CHECK(!is_complex_dtype(self.scalar_type()),
              "random_: complex receivers are not supported");
  check_random_bounds_fit_(self.scalar_type(), lo, hi);
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported = native::cuda::supports_random_stochastic(self);
    return cuda_stochastic_route_or_bridge_(
        "random_",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return random_like_cpu_(self, lo, hi).to(Platform::CUDA); });
  }
  if (self.platform() == Platform::MPS) {
    PLUSS_CHECK(hi > lo, "random_: expected hi > lo");
    PLUSS_CHECK(!is_complex_dtype(self.scalar_type()), "random_: complex receivers are not supported");
    Series u = rand_work_(self.sizes(), Platform::MPS, /*seed=*/0);
    Series x = ops_internal::floor(add_scalar(mul_scalar(u, hi - lo), lo));
    return x.to(self.scalar_type());
  }
  return random_like_cpu_(self, lo, hi);
}

static double random_default_upper_(ScalarType dtype) {
  switch (dtype) {
    case ScalarType::Bool: return 2.0;
    case ScalarType::UInt8: return 256.0;
    case ScalarType::Int8: return 128.0;
    case ScalarType::UInt16: return 65536.0;
    case ScalarType::Int16: return 32768.0;
    case ScalarType::UInt32: return 4294967296.0;
    case ScalarType::Int32: return 2147483648.0;
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32: return 16777216.0;
    case ScalarType::UInt64:
    case ScalarType::Int64:
    case ScalarType::Float64: return 9007199254740992.0;
    default: PLUSS_FAIL("random_: complex receivers are not supported");
  }
}

static Series bitshift_scalar_impl(const Series& self, std::int64_t shift, bool left) {
  auto x = self.to(ScalarType::Int64).contiguous();
  auto out = Series::empty(x.sizes(), ScalarType::Int64);
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const std::int64_t*>(x.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  if (left) {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ip[i] << shift;
  } else {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ip[i] >> shift;
  }
  return out.to(self.scalar_type());
}

static Series bitshift_series_impl(const Series& self, const Series& other, bool left) {
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  if (left) {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ap[i] << bp[i];
  } else {
    for (std::int64_t i = 0; i < n; ++i) op[i] = ap[i] >> bp[i];
  }
  return out.to(self.scalar_type());
}

static Series gcd_impl(const Series& self, const Series& other) {
  PLUSS_CHECK(self.scalar_type() != ScalarType::Bool && other.scalar_type() != ScalarType::Bool &&
                  !is_floating_dtype(self.scalar_type()) && !is_complex_dtype(self.scalar_type()) &&
                  !is_floating_dtype(other.scalar_type()) && !is_complex_dtype(other.scalar_type()),
              "gcd: integer inputs required");
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::gcd(ap[i], bp[i]);
  return out.to(promote_types(self.scalar_type(), other.scalar_type()));
}

static Series lcm_impl(const Series& self, const Series& other) {
  PLUSS_CHECK(self.scalar_type() != ScalarType::Bool && other.scalar_type() != ScalarType::Bool &&
                  !is_floating_dtype(self.scalar_type()) && !is_complex_dtype(self.scalar_type()) &&
                  !is_floating_dtype(other.scalar_type()) && !is_complex_dtype(other.scalar_type()),
              "lcm: integer inputs required");
  auto a = self.to(ScalarType::Int64);
  auto b = other.to(ScalarType::Int64);
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto aa = expand_impl(a, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto bb = expand_impl(b, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Int64);
  const auto n = out.numel();
  const auto* ap = reinterpret_cast<const std::int64_t*>(aa.raw_data());
  const auto* bp = reinterpret_cast<const std::int64_t*>(bb.raw_data());
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) op[i] = std::lcm(ap[i], bp[i]);
  return out.to(promote_types(self.scalar_type(), other.scalar_type()));
}

// fmax / fmin differ from maximum / minimum only in NaN handling: a NaN in one
// operand is ignored (the other operand wins); NaN results only when both are
// NaN. Built from where() so the backward still routes to the winning operand.
static Series fmax_fmin_impl_(const Series& a, const Series& b, bool is_max) {
  PLUSS_CHECK(!is_complex_dtype(a.scalar_type()) && !is_complex_dtype(b.scalar_type()),
              is_max ? "fmax: complex inputs are not supported"
                     : "fmin: complex inputs are not supported");
  const bool has_nan = is_floating_dtype(a.scalar_type()) || is_floating_dtype(b.scalar_type());
  if (!has_nan) return is_max ? ops_internal::maximum(a, b) : ops_internal::minimum(a, b);

  const auto dt = promote_types(a.scalar_type(), b.scalar_type());
  const Series aa = a.to(dt);
  const Series bb = b.to(dt);
  Series bn;
  {
    series_tape::NoGradGuard ng;
    bn = ops_internal::isnan(bb);
  }
  // fmax/fmin differ from maximum/minimum at both ties and NaNs. PyTorch
  // routes a finite tie to the left operand, and when both operands are NaN
  // it also routes sensitivity to the left operand.
  const Series choose_a = is_max
      ? ops_internal::logical_or(ops_internal::ge(aa, bb), bn)
      : ops_internal::logical_or(ops_internal::le(aa, bb), bn);
  return ops_internal::where(choose_a, aa, bb);
}

// float_power always computes in floating point: integer operands are promoted
// to float64 (complex128 if either side is complex) and the result stays float.
static ScalarType float_power_dtype_(ScalarType a, ScalarType b) {
  if (is_complex_dtype(a) || is_complex_dtype(b)) return ScalarType::Complex128;
  return ScalarType::Float64;
}
static Series float_power_impl_(const Series& a, const Series& b) {
  const auto dt = float_power_dtype_(a.scalar_type(), b.scalar_type());
  return ops_internal::pow(a.to(dt), b.to(dt));
}

// floor_divide rounds the quotient toward negative infinity. With two integral
// operands the result stays integral (promoted); otherwise it is floating.
static Series floor_divide_impl_(const Series& a, const Series& b) {
  Series q = ops_internal::floor(ops_internal::div(a, b));
  const bool ai = !is_floating_dtype(a.scalar_type()) && !is_complex_dtype(a.scalar_type());
  const bool bi = !is_floating_dtype(b.scalar_type()) && !is_complex_dtype(b.scalar_type());
  if (ai && bi) return q.to(promote_types(a.scalar_type(), b.scalar_type()));
  return q;
}

static Series sinc_impl(const Series& self) {
  return unary_special_impl_(self, [](double x) {
    return x == 0.0 ? 1.0 : std::sin(kPi * x) / (kPi * x);
  });
}

static Series hardshrink_impl(const Series& self, double lambda) {
  const double l = std::abs(lambda);
  // where(|x| > lambda, x, 0): vectorized, and differentiable via `where` so the
  // backward carries the gradient straight through outside [-lambda, lambda] and
  // zeroes it inside.
  const Series x = dtype_of(self.scalar_type()).is_floating()
                       ? self
                       : self.to(compat_real_dtype_for_(self));
  const Series keep = ops_internal::gt(ops_internal::abs(x), scalar_like(x, l));
  return ops_internal::where(keep, x, scalar_like(x, 0.0));
}

static Series hardshrink_backward_impl(const Series& grad_out, const Series& input, double lambda) {
  const double l = std::abs(lambda);
  auto g = grad_out.to(ScalarType::Float64);
  auto x = input.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(g.sizes(), x.sizes());
  auto gg = expand_impl(g, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto xx = expand_impl(x, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* gp = reinterpret_cast<const double*>(gg.raw_data());
  const auto* xp = reinterpret_cast<const double*>(xx.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    op[i] = (std::abs(xp[i]) > l) ? gp[i] : 0.0;
  }
  return out.to(grad_out.scalar_type());
}

static Series heaviside_impl(const Series& self, const Series& values) {
  PLUSS_CHECK(!is_complex_dtype(self.scalar_type()) && !is_complex_dtype(values.scalar_type()),
              "heaviside: complex inputs are not supported");
  PLUSS_CHECK(self.scalar_type() == values.scalar_type(),
              "heaviside: receiver and values must have the same dtype");
  auto x = self.to(ScalarType::Float64);
  auto v = values.to(ScalarType::Float64);
  auto shape = SeriesIterator::broadcast_shape(x.sizes(), v.sizes());
  auto xx = expand_impl(x, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto vv = expand_impl(v, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, ScalarType::Float64);
  const auto n = out.numel();
  const auto* xp = reinterpret_cast<const double*>(xx.raw_data());
  const auto* vp = reinterpret_cast<const double*>(vv.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    if (std::isnan(xp[i])) op[i] = xp[i];          // torch: NaN input -> NaN
    else if (xp[i] < 0.0) op[i] = 0.0;
    else if (xp[i] > 0.0) op[i] = 1.0;
    else op[i] = vp[i];
  }
  return out.to(self.scalar_type());
}

static Series aminmax_impl(const Series& self) {
  std::vector<Series> parts;
  parts.reserve(2);
  parts.push_back(ops_internal::amin(self));
  parts.push_back(ops_internal::amax(self));
  return parts[0].pile({parts[1]}, 0);
}

static Series logcumsumexp_impl(const Series& self, std::int64_t dim) {
  const Series x = self.to(ScalarType::Float64);
  if (x.dim() == 0 || x.numel() == 0) return self.clone();
  const std::int64_t d = wrap_dim(dim, x.dim(), "logcumsumexp");
  // Stable form: M + log(cumsum(exp(x - M))) with M the max along `dim`. The
  // shift makes exp() unable to overflow; the M terms cancel in the backward,
  // leaving the exact cumulative-softmax gradient.
  const Series m = ops_internal::amax(x, d, /*keepdim=*/true);
  const Series c = ops_internal::exp(ops_internal::sub(x, m)).cumsum(d, /*keepdim=*/false);
  return ops_internal::add(ops_internal::log(c), m).to(self.scalar_type());
}

static Series cross_impl(const Series& a, const Series& b, std::int64_t dim) {
  PLUSS_CHECK(a.dim() == b.dim(), "cross: expected same rank");
  PLUSS_CHECK(std::equal(a.sizes().begin(), a.sizes().end(), b.sizes().begin()), "cross: expected same shape");
  dim = wrap_dim(dim, a.dim(), "cross");
  PLUSS_CHECK(a.sizes()[(std::size_t)dim] == 3, "cross: expected dim extent == 3");

  auto a0 = a.select(dim, 0);
  auto a1 = a.select(dim, 1);
  auto a2 = a.select(dim, 2);
  auto b0 = b.select(dim, 0);
  auto b1 = b.select(dim, 1);
  auto b2 = b.select(dim, 2);

  auto c0 = ops_internal::sub(ops_internal::mul(a1, b2), ops_internal::mul(a2, b1));
  auto c1 = ops_internal::sub(ops_internal::mul(a2, b0), ops_internal::mul(a0, b2));
  auto c2 = ops_internal::sub(ops_internal::mul(a0, b1), ops_internal::mul(a1, b0));

  std::vector<Series> parts;
  parts.reserve(3);
  parts.push_back(c0);
  parts.push_back(c1);
  parts.push_back(c2);
  return parts[0].pile({parts[1], parts[2]}, dim);
}

static Series quantile_flat_impl(const Series& self, double q, bool ignore_nan) {
  PLUSS_CHECK(q >= 0.0 && q <= 1.0, "quantile: q must be between 0 and 1");
  PLUSS_CHECK(dtype_of(self.scalar_type()).is_floating(),
              "quantile: input must have a floating dtype");
  auto x = self.to(ScalarType::Float64).flatten(0, -1);
  const auto n = x.numel();
  PLUSS_CHECK(n > 0, "quantile: input must be non-empty");
  const Series host = x.contiguous();
  const auto* ip = reinterpret_cast<const double*>(host.raw_data());
  std::int64_t valid = 0;
  bool saw_nan = false;
  for (std::int64_t i = 0; i < n; ++i) {
    saw_nan = saw_nan || std::isnan(ip[i]);
    valid += !std::isnan(ip[i]);
  }
  const Series sorted = sort_values_only(x, 0, /*descending=*/false, /*stable=*/false);
  const double rank = (!ignore_nan && saw_nan)
      ? static_cast<double>(n - 1)
      : q * static_cast<double>(std::max<std::int64_t>(valid - 1, 0));
  const auto below_rank = static_cast<std::int64_t>(std::floor(rank));
  const auto above_rank = static_cast<std::int64_t>(std::ceil(rank));
  const Series below = sorted.select(0, below_rank);
  if (below_rank == above_rank) return below;
  const Series above = sorted.select(0, above_rank);
  const double weight = rank - static_cast<double>(below_rank);
  return ops_internal::add(mul_scalar(below, 1.0 - weight), mul_scalar(above, weight));
}

static Series median_flat_impl(const Series& self, bool ignore_nan) {
  PLUSS_CHECK(dtype_of(self.scalar_type()).is_floating(),
              "median: input must have a floating dtype");
  const Series x = self.to(ScalarType::Float64).flatten(0, -1);
  const Series host = x.contiguous();
  const auto n = host.numel();
  PLUSS_CHECK(n > 0, "median: input must be non-empty");
  const auto* values = reinterpret_cast<const double*>(host.raw_data());
  std::int64_t valid = 0;
  bool saw_nan = false;
  for (std::int64_t i = 0; i < n; ++i) {
    saw_nan = saw_nan || std::isnan(values[i]);
    valid += !std::isnan(values[i]);
  }

  const Series sorted = sort_values_only(x, 0, /*descending=*/false, /*stable=*/false);
  const bool result_is_nan = (!ignore_nan && saw_nan) || (ignore_nan && valid == 0);
  const auto rank = result_is_nan
      ? (ignore_nan ? 0 : n - 1)
      : (valid - 1) / 2;
  Series out = sorted.select(0, rank);
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return out;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = ignore_nan ? "nanmedian" : "median";
  link->parents = {self};
  link->propagate = [self, out, result_is_nan](const Series& out_sens) {
    Series mask = result_is_nan
        ? ops_internal::isnan(self)
        : ops_internal::eq(self, out.to(self.scalar_type()));
    Series weights = mask.to(self.scalar_type());
    const Series count = ops_internal::sum(weights);
    Series grad = ops_internal::div(
        ops_internal::mul(weights, out_sens.to(self.scalar_type())), count);
    if (grad.platform() != self.platform()) grad = grad.to(self.platform(), self.scalar_type());
    series_tape::accumulate_sensitivity(self, grad);
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}

static Series median_dim_impl(
    const Series& self,
    std::int64_t dim,
    bool keepdim,
    bool ignore_nan) {
  PLUSS_CHECK(dtype_of(self.scalar_type()).is_floating(),
              "median: input must have a floating dtype");
  const Series x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "median");
  const auto n = x.sizes()[static_cast<std::size_t>(dim)];
  PLUSS_CHECK(n > 0, "median: cannot reduce an empty dimension");
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[static_cast<std::size_t>(d)];
  for (std::int64_t d = dim + 1; d < x.dim(); ++d) inner *= x.sizes()[static_cast<std::size_t>(d)];
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  if (keepdim) out_sizes[static_cast<std::size_t>(dim)] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  Series out = Series::empty(out_sizes, ScalarType::Float64);
  Series index = Series::empty(out_sizes, ScalarType::Int64);
  const auto* input = reinterpret_cast<const double*>(x.raw_data());
  auto* output = reinterpret_cast<double*>(out.raw_mutable_data());
  auto* selected = reinterpret_cast<std::int64_t*>(index.raw_mutable_data());
  std::vector<std::pair<double, std::int64_t>> lane;
  lane.reserve(static_cast<std::size_t>(n));
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      lane.clear();
      std::int64_t first_nan = -1;
      for (std::int64_t k = 0; k < n; ++k) {
        const auto offset = (o * n + k) * inner + i;
        if (std::isnan(input[offset])) {
          if (first_nan < 0) first_nan = k;
          if (ignore_nan) continue;
        }
        lane.emplace_back(input[offset], k);
      }
      const auto destination = o * inner + i;
      if ((!ignore_nan && first_nan >= 0) || lane.empty()) {
        selected[destination] = std::max<std::int64_t>(first_nan, 0);
        output[destination] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      std::stable_sort(lane.begin(), lane.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
      });
      const auto chosen = lane[static_cast<std::size_t>((lane.size() - 1) / 2)];
      output[destination] = chosen.first;
      selected[destination] = chosen.second;
    }
  }
  return bind_selection_permutation_backward_(
      std::move(out), self, index, dim, ignore_nan ? "nanmedian" : "median");
}

static Series mode_flat_impl(const Series& self) {
  auto x = self.to(ScalarType::Float64).flatten(0, -1).contiguous();
  const auto n = x.numel();
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  PLUSS_CHECK(n > 0, "mode: cannot reduce an empty Series");
  std::vector<std::pair<double, std::int64_t>> vals(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) vals[static_cast<std::size_t>(i)] = {ip[i], i};
  std::sort(vals.begin(), vals.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });
  double best = vals[0].first;
  std::int64_t best_index = vals[0].second;
  std::int64_t bestc = 1;
  std::int64_t curc = 1;
  for (std::size_t i = 1; i < vals.size(); ++i) {
    if (vals[i].first == vals[i - 1].first) ++curc;
    else {
      if (curc > bestc) {
        bestc = curc;
        best = vals[i - 1].first;
        best_index = vals[i - 1].second;
      }
      curc = 1;
    }
  }
  if (curc > bestc) {
    best = vals.back().first;
    best_index = vals.back().second;
  }
  auto out = Series::empty({}, ScalarType::Float64);
  *reinterpret_cast<double*>(out.raw_data()) = best;
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self) &&
      dtype_of(self.scalar_type()).is_floating()) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "mode";
    link->parents = {self};
    link->propagate = [self, best_index](const Series& out_sens) {
      Series flat = Series::zeros({self.numel()}, self.scalar_type()).to(self.platform());
      const Series index = Series::full({1}, static_cast<double>(best_index), ScalarType::Int64)
                               .to(self.platform());
      const Series source = out_sens.to(self.scalar_type()).reshape({1}).to(self.platform());
      flat = flat.scatter_add(0, index, source);
      series_tape::accumulate_sensitivity(self, flat.reshape(self.sizes()));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series mode_dim_impl(const Series& self, std::int64_t dim, bool keepdim) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), "mode");
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  PLUSS_CHECK(n > 0, "mode: cannot reduce an empty dimension");
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  if (keepdim) out_sizes[(std::size_t)dim] = 1;
  else out_sizes.erase(out_sizes.begin() + dim);
  auto out = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), ScalarType::Float64);
  auto index = Series::empty(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()), ScalarType::Int64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  auto* index_out = reinterpret_cast<std::int64_t*>(index.raw_mutable_data());
  std::vector<std::pair<double, std::int64_t>> vals;
  vals.reserve(static_cast<std::size_t>(n));
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      vals.clear();
      for (std::int64_t t = 0; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        vals.emplace_back(ip[off], t);
      }
      std::sort(vals.begin(), vals.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
      });
      double best = vals[0].first;
      std::int64_t best_index = vals[0].second;
      std::int64_t bestc = 1;
      std::int64_t curc = 1;
      for (std::size_t j = 1; j < vals.size(); ++j) {
        if (vals[j].first == vals[j - 1].first) ++curc;
        else {
          if (curc > bestc) {
            bestc = curc;
            best = vals[j - 1].first;
            best_index = vals[j - 1].second;
          }
          curc = 1;
        }
      }
      if (curc > bestc) {
        best = vals.back().first;
        best_index = vals.back().second;
      }
      op[o * inner + i] = best;
      index_out[o * inner + i] = best_index;
    }
  }
  return bind_selection_permutation_backward_(
      std::move(out), self, index, dim, "mode");
}

static Series rot90_impl(const Series& self, std::int64_t k, std::int64_t d0, std::int64_t d1) {
  auto out = self;
  const auto nd = self.dim();
  d0 = wrap_dim(d0, nd, "rot90");
  d1 = wrap_dim(d1, nd, "rot90");
  PLUSS_CHECK(d0 != d1, "rot90: dims must be different");
  k %= 4;
  if (k < 0) k += 4;
  if (k == 0) return out;
  if (k == 1) {
    out = out.transpose(d0, d1);
    const std::array<std::int64_t, 1> dims{{d0}};
    return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
  }
  if (k == 2) {
    const std::array<std::int64_t, 2> dims{{d0, d1}};
    return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
  }
  out = out.transpose(d0, d1);
  const std::array<std::int64_t, 1> dims{{d1}};
  return flip_impl(out, std::span<const std::int64_t>(dims.data(), dims.size()), "rot90");
}

static Series unflatten_impl(const Series& self, std::int64_t dim, std::span<const std::int64_t> new_sizes) {
  PLUSS_CHECK(!new_sizes.empty(), "unflatten: new_sizes must be non-empty");
  dim = wrap_dim(dim, self.dim(), "unflatten");
  std::int64_t prod = 1;
  for (const auto s : new_sizes) {
    PLUSS_CHECK(s >= 0, "unflatten: sizes must be >= 0");
    prod *= s;
  }
  PLUSS_CHECK(prod == self.sizes()[(std::size_t)dim], "unflatten: product mismatch");
  std::vector<std::int64_t> out_sizes;
  out_sizes.reserve((std::size_t)(self.dim() - 1 + static_cast<std::int64_t>(new_sizes.size())));
  for (std::int64_t d = 0; d < dim; ++d) out_sizes.push_back(self.sizes()[(std::size_t)d]);
  out_sizes.insert(out_sizes.end(), new_sizes.begin(), new_sizes.end());
  for (std::int64_t d = dim + 1; d < self.dim(); ++d) out_sizes.push_back(self.sizes()[(std::size_t)d]);
  return self.reshape(std::span<const std::int64_t>(out_sizes.data(), out_sizes.size()));
}

static Series unfold_impl(const Series& self, std::int64_t dim, std::int64_t size, std::int64_t step) {
  PLUSS_CHECK(size > 0 && step > 0, "unfold: size and step must be > 0");
  const Series x = self.contiguous();
  dim = wrap_dim(dim, x.dim(), "unfold");
  const auto n = x.sizes()[(std::size_t)dim];
  PLUSS_CHECK(size <= n, "unfold: size cannot exceed the selected dimension");
  const auto m = 1 + (n - size) / step;
  std::vector<std::int64_t> out_sizes(x.sizes().begin(), x.sizes().end());
  std::vector<std::int64_t> out_strides(x.strides().begin(), x.strides().end());
  out_sizes.reserve((std::size_t)(x.dim() + 1));
  out_strides.reserve((std::size_t)(x.dim() + 1));
  out_sizes[(std::size_t)dim] = m;
  out_strides[(std::size_t)dim] *= step;
  out_sizes.push_back(size);
  out_strides.push_back(x.strides()[(std::size_t)dim]);
  const auto offset = x.storage_offset() / std::max<std::int64_t>(1, x.itemsize());
  return as_strided_impl(x, out_sizes, out_strides, offset);
}

static Series sum_to_size_impl(const Series& self, std::span<const std::int64_t> target) {
  auto out = self;
  PLUSS_CHECK(target.size() <= static_cast<std::size_t>(out.dim()), "sum_to_size: target rank must be <= input rank");
  while (out.dim() > static_cast<std::int64_t>(target.size())) {
    out = ops_internal::sum(out, 0, false);
  }
  for (std::int64_t d = 0; d < out.dim(); ++d) {
    const auto t = target[(std::size_t)d];
    const auto s = out.sizes()[(std::size_t)d];
    if (t == s) continue;
    PLUSS_CHECK(t == 1, "sum_to_size: incompatible target dimension");
    out = ops_internal::sum(out, d, true);
  }
  return out.reshape(target);
}

static bool cpu_sorting_kernel_supports_dtype_(ScalarType st) {
  switch (st) {
    case ScalarType::Float64:
    case ScalarType::Float32:
    case ScalarType::Int64:
    case ScalarType::Int32:
    case ScalarType::Int16:
    case ScalarType::Int8:
    case ScalarType::UInt8:
    case ScalarType::UInt16:
    case ScalarType::UInt32:
    case ScalarType::UInt64:
    case ScalarType::Bool:
      return true;
    default:
      return false;
  }
}

static bool parse_bool_env_(const char* name, bool default_value) {
  const char* raw = std::getenv(name);
  if (raw == nullptr) {
    return default_value;
  }
  const auto eq = [raw](const char* lit) {
    const unsigned char* a = reinterpret_cast<const unsigned char*>(raw);
    const unsigned char* b = reinterpret_cast<const unsigned char*>(lit);
    while (*a != '\0' && *b != '\0') {
      unsigned char ca = *a;
      unsigned char cb = *b;
      if (ca >= 'A' && ca <= 'Z') ca = static_cast<unsigned char>(ca - 'A' + 'a');
      if (cb >= 'A' && cb <= 'Z') cb = static_cast<unsigned char>(cb - 'A' + 'a');
      if (ca != cb) return false;
      ++a;
      ++b;
    }
    return *a == '\0' && *b == '\0';
  };
  if (eq("1") || eq("true") || eq("yes") || eq("on")) return true;
  if (eq("0") || eq("false") || eq("no") || eq("off")) return false;
  return default_value;
}

static bool mps_native_sort_selection_enabled_() {
  return parse_bool_env_("PLUSS_MPS_NATIVE_SORT_SELECTION", true);
}

static bool mps_native_sort_selection_strict_() {
  return parse_bool_env_("PLUSS_MPS_NATIVE_SORT_SELECTION_STRICT", false);
}

static bool mps_native_topk_selection_enabled_() {
  return parse_bool_env_("PLUSS_MPS_NATIVE_TOPK_SELECTION", true);
}

static std::pair<Series, bool> prepare_cpu_sort_input_(const Series& self) {
  auto cpu = self.to(Platform::CPU);
  if (cpu_sorting_kernel_supports_dtype_(cpu.scalar_type())) {
    return {cpu, false};
  }
  PLUSS_CHECK(
      dtype_of(cpu.scalar_type()).is_floating() && !dtype_of(cpu.scalar_type()).is_complex(),
      std::string("sorting fallback: unsupported dtype ") + std::string(dtype_of(cpu.scalar_type()).name));
  return {cpu.to(ScalarType::Float32), true};
}

static Series restore_sorted_value_dtype_(Series values_cpu, const Series& self, bool casted_for_sorting) {
  if (casted_for_sorting) {
    values_cpu = values_cpu.to(self.scalar_type());
  }
  return values_cpu;
}

struct MpsSelectionCacheKey {
  const pluss::codem::SeriesImpl* impl_ptr{nullptr};
  std::uint64_t impl_instance_id{0};
  const void* storage_handle{nullptr};
  ScalarType dtype{ScalarType::Float32};
  const void* raw_ptr{nullptr};
  std::int64_t storage_version{0};
  std::int64_t storage_offset{0};
  std::uint64_t layout_signature{0};
  std::int64_t dim{0};
  std::int64_t k{0};
  bool flag0{false};
  bool flag1{false};
  bool flag2{false};
};

struct MpsSelectionCacheEntry {
  bool valid{false};
  MpsSelectionCacheKey key{};
  Series value{Series::undefined()};
};

static std::uint64_t hash_layout_signature_(const Series& self) {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t h = kOffset;
  h ^= static_cast<std::uint64_t>(self.dim());
  h *= kPrime;
  for (std::int64_t d = 0; d < self.dim(); ++d) {
    h ^= static_cast<std::uint64_t>(self.sizes()[static_cast<std::size_t>(d)]);
    h *= kPrime;
    h ^= static_cast<std::uint64_t>(self.strides()[static_cast<std::size_t>(d)]);
    h *= kPrime;
  }
  h ^= static_cast<std::uint64_t>(self.numel());
  h *= kPrime;
  return h;
}

static MpsSelectionCacheKey make_mps_selection_cache_key_(
    const Series& self,
    std::int64_t dim,
    std::int64_t k,
    bool flag0,
    bool flag1,
    bool flag2) {
  MpsSelectionCacheKey key{};
  key.impl_ptr = self.unsafe_impl();
  key.impl_instance_id = (key.impl_ptr != nullptr) ? key.impl_ptr->instance_id() : 0;
  key.storage_handle = self.storage().handle().get();
  key.dtype = self.scalar_type();
  key.raw_ptr =
      (self.platform() == Platform::MPS || key.impl_ptr == nullptr) ? nullptr : static_cast<const void*>(key.impl_ptr->raw_data());
  key.storage_version = self.storage_version();
  key.storage_offset = self.storage_offset();
  key.layout_signature = hash_layout_signature_(self);
  key.dim = dim;
  key.k = k;
  key.flag0 = flag0;
  key.flag1 = flag1;
  key.flag2 = flag2;
  return key;
}

static bool mps_selection_cache_matches_(
    const MpsSelectionCacheEntry& entry,
    const Series& self,
    std::int64_t dim,
    std::int64_t k,
    bool flag0,
    bool flag1,
    bool flag2) {
  if (!entry.valid) {
    return false;
  }
  const auto key = make_mps_selection_cache_key_(self, dim, k, flag0, flag1, flag2);
  return entry.key.impl_ptr == key.impl_ptr &&
      entry.key.impl_instance_id == key.impl_instance_id &&
      entry.key.storage_handle == key.storage_handle &&
      entry.key.dtype == key.dtype &&
      entry.key.raw_ptr == key.raw_ptr &&
      entry.key.storage_version == key.storage_version &&
      entry.key.storage_offset == key.storage_offset &&
      entry.key.layout_signature == key.layout_signature &&
      entry.key.dim == key.dim &&
      entry.key.k == key.k &&
      entry.key.flag0 == key.flag0 &&
      entry.key.flag1 == key.flag1 &&
      entry.key.flag2 == key.flag2;
}

static Series sort_values_only_impl_(const Series& self, std::int64_t dim, bool descending, bool stable) {
  dim = wrap_dim(dim, self.dim(), "sort");
  if (self.platform() == Platform::MPS) {
    static thread_local MpsSelectionCacheEntry cache{};
    if (mps_selection_cache_matches_(cache, self, dim, /*k=*/0, descending, stable, false)) {
      return cache.value.clone();
    }
    const bool native_enabled = mps_native_sort_selection_enabled_();
    const bool native_strict = mps_native_sort_selection_strict_();
    if (native_strict && self.scalar_type() != ScalarType::Float32) {
      throw_error("sort: strict native MPS route rejects cast-assisted dtype lane");
    }
    if (native_enabled && native::mps::supports_sort_selection(self, dim, stable)) {
      try {
        Series out = native::mps::sort_with_indices(self, dim, descending).first;
        cache.valid = true;
        cache.key = make_mps_selection_cache_key_(self, dim, /*k=*/0, descending, stable, false);
        cache.value = out.clone();
        return out;
      } catch (const std::exception& e) {
        if (native_strict) {
          throw_error(std::string("sort: strict native MPS route failed: ") + e.what());
        }
      } catch (...) {
        if (native_strict) {
          throw_error("sort: strict native MPS route failed");
        }
      }
    } else if (native_strict) {
      throw_error("sort: strict native MPS route requested but kernel is unsupported");
    }
    auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
    auto pair = native::cpu::sort(cpu, dim, descending, stable);
    auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
    Series out = values.to(Platform::MPS);
    cache.valid = true;
    cache.key = make_mps_selection_cache_key_(self, dim, /*k=*/0, descending, stable, false);
    cache.value = out.clone();
    return out;
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_selection_enabled();
    const bool native_strict = native::cuda::native_selection_strict();
    const bool route_supported = native::cuda::supports_sort_selection(self, dim, stable);
    const bool bridge_enabled = native::cuda::cpu_bridge_enabled();
    if (native_enabled && route_supported) {
      if (!bridge_enabled) {
        throw_error("sort: CUDA native selection route declared but kernel is not implemented yet");
      }
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::sort(cpu, dim, descending, stable);
      auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
      return values.to(Platform::CUDA);
    }
    if (native_strict) {
      throw_error("sort: strict native CUDA route requested but kernel is unsupported");
    }
    if (bridge_enabled) {
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::sort(cpu, dim, descending, stable);
      auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
      return values.to(Platform::CUDA);
    }
    throw_error(
        "sort: CUDA path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or implement CUDA selection kernel.");
  }
  auto pair = native::cpu::sort(self, dim, descending, stable);
  return pair.first;
}

static Series bind_selection_permutation_backward_(
    Series out,
    const Series& self,
    const Series& gather_index,
    std::int64_t dim,
    const char* op_name);

static Series topk_values_only_impl_(const Series& self, std::int64_t k, std::int64_t dim, bool largest, bool sorted) {
  dim = wrap_dim(dim, self.dim(), "topk");
  if (self.platform() == Platform::MPS) {
    static thread_local MpsSelectionCacheEntry cache{};
    if (mps_selection_cache_matches_(cache, self, dim, k, largest, sorted, false)) {
      return cache.value.clone();
    }
    const bool native_enabled = mps_native_sort_selection_enabled_();
    const bool native_topk_enabled = mps_native_topk_selection_enabled_();
    const bool native_strict = mps_native_sort_selection_strict_();
    if (native_strict && self.scalar_type() != ScalarType::Float32) {
      throw_error("topk: strict native MPS route rejects cast-assisted dtype lane");
    }
    if (native_enabled && native_topk_enabled && sorted && k > 0 &&
        native::mps::supports_sort_selection(self, dim, /*stable=*/false)) {
      try {
        Series out = native::mps::topk_with_indices(self, k, dim, largest).first;
        cache.valid = true;
        cache.key = make_mps_selection_cache_key_(self, dim, k, largest, sorted, false);
        cache.value = out.clone();
        return out;
      } catch (const std::exception& e) {
        if (native_strict) {
          throw_error(std::string("topk: strict native MPS route failed: ") + e.what());
        }
      } catch (...) {
        if (native_strict) {
          throw_error("topk: strict native MPS route failed");
        }
      }
    } else if (native_strict && native_topk_enabled && sorted && k > 0) {
      throw_error("topk: strict native MPS route requested but kernel is unsupported");
    }
    auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
    auto pair = native::cpu::topk(cpu, k, dim, largest, sorted);
    auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
    Series out = values.to(Platform::MPS);
    cache.valid = true;
    cache.key = make_mps_selection_cache_key_(self, dim, k, largest, sorted, false);
    cache.value = out.clone();
    return out;
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_selection_enabled();
    const bool native_strict = native::cuda::native_selection_strict();
    const bool route_supported = native::cuda::supports_topk_selection(self, k, dim, sorted);
    const bool bridge_enabled = native::cuda::cpu_bridge_enabled();
    if (native_enabled && route_supported) {
      if (!bridge_enabled) {
        throw_error("topk: CUDA native selection route declared but kernel is not implemented yet");
      }
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::topk(cpu, k, dim, largest, sorted);
      auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
      return values.to(Platform::CUDA);
    }
    if (native_strict) {
      throw_error("topk: strict native CUDA route requested but kernel is unsupported");
    }
    if (bridge_enabled) {
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::topk(cpu, k, dim, largest, sorted);
      auto values = restore_sorted_value_dtype_(pair.first, self, casted_for_sorting);
      return values.to(Platform::CUDA);
    }
    throw_error(
        "topk: CUDA path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or implement CUDA selection kernel.");
  }
  PLUSS_CHECK(self.platform() == Platform::CPU, "topk: unsupported platform");
  auto pair = native::cpu::topk(self, k, dim, largest, sorted);
  return bind_selection_permutation_backward_(
      std::move(pair.first), self, pair.second, dim, "topk");
}

static Series bind_kthvalue_backward_(
    Series out,
    const Series& self,
    const Series& index_keepdim,
    std::int64_t dim,
    bool keepdim) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return out;
  if (!dtype_of(self.scalar_type()).is_floating()) return out;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "kthvalue";
  link->parents = {self};
  link->propagate = [self, index_keepdim, dim, keepdim](const Series& out_sens) {
    Series sens = keepdim ? out_sens : out_sens.unsqueeze(dim);
    sens = sens.to(self.platform()).to(self.scalar_type()).contiguous();
    const Series index = index_keepdim.to(self.platform()).contiguous();
    Series grad = Series::zeros(self.sizes(), self.scalar_type(), self.platform())
                      .scatter_add(dim, index, sens);
    series_tape::accumulate_sensitivity(self, grad);
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}

static Series kthvalue_values_only(const Series& self, std::int64_t k, std::int64_t dim, bool keepdim) {
  PLUSS_CHECK(k >= 1, "kthvalue: k must be >= 1");
  dim = wrap_dim(dim, self.dim(), "kthvalue");
  PLUSS_CHECK(k <= self.sizes()[(std::size_t)dim], "kthvalue: k must be <= size(dim)");
  if (self.platform() == Platform::MPS) {
    static thread_local MpsSelectionCacheEntry cache{};
    if (mps_selection_cache_matches_(cache, self, dim, k, keepdim, false, false)) {
      return cache.value.clone();
    }
    const bool native_enabled = mps_native_sort_selection_enabled_();
    const bool native_topk_enabled = mps_native_topk_selection_enabled_();
    const bool native_strict = mps_native_sort_selection_strict_();
    if (native_strict && self.scalar_type() != ScalarType::Float32) {
      throw_error("kthvalue: strict native MPS route rejects cast-assisted dtype lane");
    }
    if (native_enabled && native_topk_enabled && native::mps::supports_sort_selection(self, dim, /*stable=*/false)) {
      try {
        Series value = native::mps::kthvalue(self, k, dim, keepdim);
        cache.valid = true;
        cache.key = make_mps_selection_cache_key_(self, dim, k, keepdim, false, false);
        cache.value = value.clone();
        return value;
      } catch (const std::exception& e) {
        if (native_strict) {
          throw_error(std::string("kthvalue: strict native MPS route failed: ") + e.what());
        }
      } catch (...) {
        if (native_strict) {
          throw_error("kthvalue: strict native MPS route failed");
        }
      }
    } else if (native_strict && native_topk_enabled) {
      throw_error("kthvalue: strict native MPS route requested but kernel is unsupported");
    }
    auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
    auto pair = native::cpu::topk(cpu, k, dim, /*largest=*/false, /*sorted=*/true);
    Series value = pair.first.select(dim, k - 1);
    if (keepdim) value = value.unsqueeze(dim);
    value = restore_sorted_value_dtype_(std::move(value), self, casted_for_sorting);
    Series out = value.to(Platform::MPS);
    cache.valid = true;
    cache.key = make_mps_selection_cache_key_(self, dim, k, keepdim, false, false);
    cache.value = out.clone();
    return out;
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_selection_enabled();
    const bool native_strict = native::cuda::native_selection_strict();
    const bool route_supported = native::cuda::supports_kthvalue_selection(self, k, dim);
    const bool bridge_enabled = native::cuda::cpu_bridge_enabled();
    if (native_enabled && route_supported) {
      if (!bridge_enabled) {
        throw_error("kthvalue: CUDA native selection route declared but kernel is not implemented yet");
      }
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::topk(cpu, k, dim, /*largest=*/false, /*sorted=*/true);
      Series value = pair.first.select(dim, k - 1);
      if (keepdim) value = value.unsqueeze(dim);
      value = restore_sorted_value_dtype_(std::move(value), self, casted_for_sorting);
      return value.to(Platform::CUDA);
    }
    if (native_strict) {
      throw_error("kthvalue: strict native CUDA route requested but kernel is unsupported");
    }
    if (bridge_enabled) {
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      auto pair = native::cpu::topk(cpu, k, dim, /*largest=*/false, /*sorted=*/true);
      Series value = pair.first.select(dim, k - 1);
      if (keepdim) value = value.unsqueeze(dim);
      value = restore_sorted_value_dtype_(std::move(value), self, casted_for_sorting);
      return value.to(Platform::CUDA);
    }
    throw_error(
        "kthvalue: CUDA path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or implement CUDA selection kernel.");
  }
  PLUSS_CHECK(self.platform() == Platform::CPU, "kthvalue: unsupported platform");
  auto pair = native::cpu::topk(self, k, dim, /*largest=*/false, /*sorted=*/true);
  Series index = pair.second.select(dim, k - 1).unsqueeze(dim).contiguous();
  Series value = pair.first.select(dim, k - 1);
  if (keepdim) value = value.unsqueeze(dim);
  return bind_kthvalue_backward_(std::move(value), self, index, dim, keepdim);
}

static Series argsort_values_only(const Series& self, std::int64_t dim, bool descending, bool stable) {
  dim = wrap_dim(dim, self.dim(), "argsort");
  if (self.platform() == Platform::MPS) {
    static thread_local MpsSelectionCacheEntry cache{};
    if (mps_selection_cache_matches_(cache, self, dim, /*k=*/0, descending, stable, true)) {
      return cache.value.clone();
    }
    const bool native_enabled = mps_native_sort_selection_enabled_();
    const bool native_strict = mps_native_sort_selection_strict_();
    if (native_strict && self.scalar_type() != ScalarType::Float32) {
      throw_error("argsort: strict native MPS route rejects cast-assisted dtype lane");
    }
    if (native_enabled && !stable && native::mps::supports_sort_selection(self, dim, stable)) {
      try {
        Series out = native::mps::argsort(self, dim, descending);
        cache.valid = true;
        cache.key = make_mps_selection_cache_key_(self, dim, /*k=*/0, descending, stable, true);
        cache.value = out.clone();
        return out;
      } catch (const std::exception& e) {
        if (native_strict) {
          throw_error(std::string("argsort: strict native MPS route failed: ") + e.what());
        }
      } catch (...) {
        if (native_strict) {
          throw_error("argsort: strict native MPS route failed");
        }
      }
    } else if (native_strict) {
      throw_error("argsort: strict native MPS route requested but kernel is unsupported");
    }
    auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
    (void)casted_for_sorting;
    Series out = native::cpu::argsort(cpu, dim, descending, stable).to(Platform::MPS);
    cache.valid = true;
    cache.key = make_mps_selection_cache_key_(self, dim, /*k=*/0, descending, stable, true);
    cache.value = out.clone();
    return out;
  }
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_selection_enabled();
    const bool native_strict = native::cuda::native_selection_strict();
    const bool route_supported = native::cuda::supports_argsort_selection(self, dim, stable);
    const bool bridge_enabled = native::cuda::cpu_bridge_enabled();
    if (native_enabled && route_supported) {
      if (!bridge_enabled) {
        throw_error("argsort: CUDA native selection route declared but kernel is not implemented yet");
      }
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      (void)casted_for_sorting;
      return native::cpu::argsort(cpu, dim, descending, stable).to(Platform::CUDA);
    }
    if (native_strict) {
      throw_error("argsort: strict native CUDA route requested but kernel is unsupported");
    }
    if (bridge_enabled) {
      auto [cpu, casted_for_sorting] = prepare_cpu_sort_input_(self);
      (void)casted_for_sorting;
      return native::cpu::argsort(cpu, dim, descending, stable).to(Platform::CUDA);
    }
    throw_error(
        "argsort: CUDA path unavailable in this tranche; no CPU fallback allowed. "
        "Move Series to CPU explicitly or implement CUDA selection kernel.");
  }
  PLUSS_CHECK(self.platform() == Platform::CPU, "argsort: unsupported platform");
  return native::cpu::argsort(self, dim, descending, stable);
}

// Value-channel backward for a selection-by-permutation op (`sort` / `topk`).
// The forward gathers `out[..i..] = self[.. gather_index[..i..] ..]` along
// `dim`; the ordering itself is a discrete, non-differentiable function of the
// data, but the values pass straight through a fixed permutation, so the
// backward simply scatters the upstream gradient back to the positions it was
// gathered from. Vectorized: one `scatter_add` into a zero buffer, no host
// loop. `gather_index` must be the index set that produced `out` (a full
// per-lane permutation for `sort`, the first-`k` slice of it for `topk`).
static Series bind_selection_permutation_backward_(Series out,
                                                   const Series& self,
                                                   const Series& gather_index,
                                                   std::int64_t dim,
                                                   const char* op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return out;
  if (!dtype_of(self.scalar_type()).is_floating()) return out;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name;
  link->parents = {self};
  const std::int64_t d = dim;
  link->propagate = [self, gather_index, d](const Series& out_sens) {
    Series idx = gather_index.to(self.platform()).contiguous();
    Series g = out_sens.to(self.platform()).to(self.scalar_type()).contiguous();
    if (g.dim() + 1 == self.dim()) g = g.unsqueeze(d);
    if (idx.dim() + 1 == self.dim()) idx = idx.unsqueeze(d);
    Series grad_in =
        Series::zeros(self.sizes(), self.scalar_type(), self.platform())
            .scatter_add(d, idx, g);
    series_tape::accumulate_sensitivity(self, grad_in);
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}

static Series sort_values_only(const Series& self, std::int64_t dim, bool descending, bool stable) {
  Series values = sort_values_only_impl_(self, dim, descending, stable);
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self) &&
      dtype_of(self.scalar_type()).is_floating()) {
    const std::int64_t d = wrap_dim(dim, self.dim(), "sort");
    Series perm;
    {
      // The index computation must not itself record onto the tape, but the
      // link below must be bound with recording still active -- so the guard
      // scope closes before bind_selection_permutation_backward_ runs.
      series_tape::NoGradGuard ng;
      perm = argsort_values_only(self, d, descending, stable);
    }
    values = bind_selection_permutation_backward_(std::move(values), self, perm, d, "sort");
  }
  return values;
}

static Series topk_values_only(const Series& self, std::int64_t k, std::int64_t dim, bool largest, bool sorted) {
  Series values = topk_values_only_impl_(self, k, dim, largest, sorted);
  if (self.platform() != Platform::CPU && sorted && k > 0 && series_tape::recording_enabled() &&
      series_tape::tracks_slope(self) &&
      dtype_of(self.scalar_type()).is_floating()) {
    const std::int64_t d = wrap_dim(dim, self.dim(), "topk");
    Series idx;
    {
      series_tape::NoGradGuard ng;
      // `topk(sorted=true)` returns the k selected values in the same order the
      // full descending/ascending argsort would place them, so the first k
      // entries of that argsort are exactly the source positions.
      Series order = argsort_values_only(self, d, /*descending=*/largest, /*stable=*/false);
      idx = order.slice(d, 0, k).contiguous();
    }
    values = bind_selection_permutation_backward_(std::move(values), self, idx, d, "topk");
  }
  return values;
}

static Series tril_triu_impl(const Series& self, std::int64_t diagonal, bool upper) {
  PLUSS_CHECK(self.dim() >= 2, "tril/triu: expected tensor with at least 2 dimensions");
  auto x = self.contiguous();
  auto out = Series::zeros(x.sizes(), x.scalar_type());

  const auto nd = x.dim();
  const auto rows = x.sizes()[(std::size_t)(nd - 2)];
  const auto cols = x.sizes()[(std::size_t)(nd - 1)];
  const auto item = x.dtype().itemsize;
  const auto batches = (rows == 0 || cols == 0) ? 0 : (x.numel() / (rows * cols));

  const auto* src = x.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t b = 0; b < batches; ++b) {
    for (std::int64_t r = 0; r < rows; ++r) {
      std::int64_t lo = 0;
      std::int64_t hi = 0;
      if (!upper) {
        lo = 0;
        hi = std::clamp<std::int64_t>(r + diagonal + 1, 0, cols);
      } else {
        lo = std::clamp<std::int64_t>(r + diagonal, 0, cols);
        hi = cols;
      }
      if (hi <= lo) continue;
      const auto row_base = (b * rows + r) * cols;
      const auto byte_off = (row_base + lo) * item;
      const auto nbytes = static_cast<std::size_t>((hi - lo) * item);
      std::memcpy(dst + byte_off, src + byte_off, nbytes);
    }
  }
  bind_triangular_mask_backward_(out, self, diagonal, upper, upper ? "triu" : "tril");
  return out;
}

static Series cast_like_(const Series& value, const Series& like) {
  Series out = value;
  if (out.scalar_type() != like.scalar_type()) out = out.to(like.scalar_type());
  if (out.platform() != like.platform()) out = out.to(like.platform(), like.scalar_type());
  return out;
}

static void bind_triangular_mask_backward_(
    const Series& out,
    const Series& self,
    std::int64_t diagonal,
    bool upper,
    const char* op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return;

  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name;
  link->parents = {self};
  link->propagate = [self, diagonal, upper](const Series& out_sens) {
    Series grad = tril_triu_impl(cast_like_(out_sens, self), diagonal, upper);
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series diag_extract_2d(const Series& self, std::int64_t diagonal) {
  PLUSS_CHECK(self.dim() == 2, "diag/diagonal: extraction expects a 2D input");
  auto x = self.contiguous();
  const auto rows = x.sizes()[0];
  const auto cols = x.sizes()[1];
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto len = std::max<std::int64_t>(0, std::min(rows - row0, cols - col0));
  auto out = Series::empty({len}, x.scalar_type());
  const auto item = x.dtype().itemsize;
  const auto* src = x.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t i = 0; i < len; ++i) {
    const auto src_off = ((row0 + i) * cols + (col0 + i)) * item;
    const auto dst_off = i * item;
    std::memcpy(dst + dst_off, src + src_off, static_cast<std::size_t>(item));
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "diag_extract";
    link->parents = {self};
    link->propagate = [self, diagonal](const Series& out_sens) {
      Series base = Series::zeros(self.sizes(), self.scalar_type()).to(self.platform());
      Series grad = diagonal_scatter_impl(base, cast_like_(out_sens, self), diagonal);
      series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series diag_embed_from_1d(const Series& self, std::int64_t diagonal) {
  auto v = self.flatten(0, -1).contiguous();
  PLUSS_CHECK(v.dim() == 1, "diag_embed/diagflat: expected 1D after flatten");
  const auto n = v.numel();
  const auto side = n + std::llabs(diagonal);
  auto out = Series::zeros({side, side}, v.scalar_type());
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto item = v.dtype().itemsize;
  const auto* src = v.raw_data();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_data());
  for (std::int64_t i = 0; i < n; ++i) {
    const auto src_off = i * item;
    const auto dst_off = ((row0 + i) * side + (col0 + i)) * item;
    std::memcpy(dst + dst_off, src + src_off, static_cast<std::size_t>(item));
  }
  if (series_tape::recording_enabled() && series_tape::tracks_slope(self)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "diag_embed";
    link->parents = {self};
    link->propagate = [self, diagonal](const Series& out_sens) {
      Series grad = diag_extract_2d(cast_like_(out_sens, self), diagonal);
      if (std::vector<std::int64_t>(grad.sizes().begin(), grad.sizes().end()) !=
          std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end())) {
        grad = grad.reshape(std::vector<std::int64_t>(self.sizes().begin(), self.sizes().end()));
      }
      series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series isclose_impl(const Series& a, const Series& b, double rtol, double atol, bool equal_nan) {
  PLUSS_CHECK(a.scalar_type() == b.scalar_type(),
              "isclose: receiver and other must have the same dtype");
  PLUSS_CHECK(a.platform() == b.platform(),
              "isclose: receiver and other must use the same platform");
  PLUSS_CHECK(rtol >= 0.0, "isclose: rtol must be greater than or equal to zero");
  PLUSS_CHECK(atol >= 0.0, "isclose: atol must be greater than or equal to zero");

  Series close = ops_internal::eq(a, b);
  if (equal_nan) {
    Series both_nan = ops_internal::logical_and(ops_internal::isnan(a), ops_internal::isnan(b));
    close = ops_internal::logical_or(close, both_nan);
  }
  if (rtol == 0.0 && atol == 0.0) return close;

  Series aa = a;
  Series bb = b;
  if (a.scalar_type() == ScalarType::Bool || dtype_of(a.scalar_type()).is_integral()) {
    aa = a.to(ScalarType::Float32);
    bb = b.to(ScalarType::Float32);
  }
  const Series actual_error = ops_internal::abs(ops_internal::sub(aa, bb));
  const Series allowed_error =
      add_scalar(mul_scalar(ops_internal::abs(bb), rtol), atol);
  const Series finite_close = ops_internal::logical_and(
      ops_internal::isfinite(actual_error), ops_internal::le(actual_error, allowed_error));
  return ops_internal::logical_or(close, finite_close);
}

// Backward of a running extremum: the sensitivity of output position t flows
// entirely to the input position that held the running max/min at t. Equal
// values select the newest position, and a NaN becomes the selected value,
// matching the value/index pair produced by PyTorch's cumulative kernels.
static void bind_cum_extrema_backward_(
    const Series& out,
    const Series& self,
    std::int64_t dim,
    bool is_max) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(self)) return;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = is_max ? "cummax" : "cummin";
  link->parents = {self};
  link->propagate = [self, dim, is_max](const Series& out_sens) {
    const Series x = self.to(Platform::CPU).to(ScalarType::Float64).contiguous();
    const Series incoming = out_sens.to(Platform::CPU).to(ScalarType::Float64).contiguous();
    Series grad = Series::zeros(self.sizes(), ScalarType::Float64);
    const auto d = wrap_dim(dim, x.dim(), is_max ? "cummax" : "cummin");
    const auto nd = x.dim();
    const auto n = x.sizes()[static_cast<std::size_t>(d)];
    std::int64_t outer = 1;
    std::int64_t inner = 1;
    for (std::int64_t axis = 0; axis < d; ++axis) outer *= x.sizes()[static_cast<std::size_t>(axis)];
    for (std::int64_t axis = d + 1; axis < nd; ++axis) inner *= x.sizes()[static_cast<std::size_t>(axis)];
    const double* xp = reinterpret_cast<const double*>(x.raw_data());
    const double* gp = reinterpret_cast<const double*>(incoming.raw_data());
    double* rp = reinterpret_cast<double*>(grad.raw_data());
    for (std::int64_t o = 0; o < outer; ++o) {
      for (std::int64_t i = 0; i < inner; ++i) {
        std::int64_t arg = 0;
        double cur = xp[(o * n + 0) * inner + i];
        rp[(o * n + 0) * inner + i] += gp[(o * n + 0) * inner + i];
        for (std::int64_t t = 1; t < n; ++t) {
          const auto off = (o * n + t) * inner + i;
          const double v = xp[off];
          if (std::isnan(v) || (!std::isnan(cur) && (is_max ? (v >= cur) : (v <= cur)))) {
            cur = v;
            arg = t;
          }
          rp[(o * n + arg) * inner + i] += gp[off];
        }
      }
    }
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
}

static Series cum_extrema_impl(const Series& self, std::int64_t dim, bool is_max) {
  auto x = self.to(ScalarType::Float64).contiguous();
  dim = wrap_dim(dim, x.dim(), is_max ? "cummax" : "cummin");
  auto out = Series::empty(x.sizes(), ScalarType::Float64);
  const auto* ip = reinterpret_cast<const double*>(x.raw_data());
  auto* op = reinterpret_cast<double*>(out.raw_data());
  const auto nd = x.dim();
  const auto n = x.sizes()[(std::size_t)dim];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < nd; ++d) inner *= x.sizes()[(std::size_t)d];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t i = 0; i < inner; ++i) {
      double cur = ip[(o * n + 0) * inner + i];
      op[(o * n + 0) * inner + i] = cur;
      for (std::int64_t t = 1; t < n; ++t) {
        const auto off = (o * n + t) * inner + i;
        const double value = ip[off];
        if (std::isnan(value) || (!std::isnan(cur) && (is_max ? value >= cur : value <= cur))) {
          cur = value;
        }
        op[off] = cur;
      }
    }
  }
  Series result = out.to(self.scalar_type());
  bind_cum_extrema_backward_(result, self, dim, is_max);
  return result;
}

static Series kron_impl(const Series& a, const Series& b) {
  // Kronecker product as a broadcast-multiply of interleaved shapes, so it
  // stays differentiable through `mul` + `reshape` (no host loop).
  const ScalarType rt = compat_real_dtype_for_(a);
  if (a.dim() == 2 && b.dim() == 2) {
    const auto m = a.sizes()[0], n = a.sizes()[1];
    const auto p = b.sizes()[0], q = b.sizes()[1];
    Series prod = ops_internal::mul(a.to(rt).reshape({m, 1, n, 1}),
                                    b.to(rt).reshape({1, p, 1, q}));
    return prod.reshape({m * p, n * q});
  }
  // General fallback: outer product of the flattened operands, then reshape to
  // the concatenation of the two input shapes (matches the prior behavior).
  const Series af = a.to(rt).flatten(0, -1);
  const Series bf = b.to(rt).flatten(0, -1);
  const std::int64_t an = af.numel();
  const std::int64_t bn = bf.numel();
  Series prod = ops_internal::mul(af.reshape({an, 1}), bf.reshape({1, bn}));
  std::vector<std::int64_t> shape;
  shape.reserve((std::size_t)(a.dim() + b.dim()));
  shape.insert(shape.end(), a.sizes().begin(), a.sizes().end());
  shape.insert(shape.end(), b.sizes().begin(), b.sizes().end());
  return prod.reshape(std::span<const std::int64_t>(shape.data(), shape.size()));
}

// cov(X): treat each row of a 2D X as a variable observed across the columns
// (a 1D X is one variable). Centre each variable, then X_c @ X_c^T scaled by
// 1 / (num_observations - correction). Built through matmul_with_tape_ so the
// whole thing is differentiable.
static Series covariance_impl(const Series& self, std::int64_t correction = 1) {
  auto x = self.to(ScalarType::Float64);
  PLUSS_CHECK(x.dim() == 1 || x.dim() == 2, "cov: expected a 1D or 2D input");
  if (x.dim() == 1) {
    const double denom = static_cast<double>(x.numel()) - static_cast<double>(correction);
    Series xc = centered_impl(x);
    Series v = div_scalar(ops_internal::sum(ops_internal::square(xc)), denom);
    return v.reshape(std::vector<std::int64_t>{});
  }
  const auto num_obs = x.sizes()[1];
  const double denom = static_cast<double>(num_obs) - static_cast<double>(correction);
  Series xc = centered_impl(x, /*dim=*/1);
  Series c = matmul_with_tape_(xc, xc.transpose(0, 1));
  return div_scalar(c, denom);
}

// corrcoef(X): cov(X) with each entry divided by the geometric mean of the two
// variables' variances, so the diagonal is 1 and every entry lies in [-1, 1].
// The (num_obs - correction) factor cancels in the ratio, so correction is
// irrelevant here. Vectorized and differentiable.
static Series corrcoef_impl(const Series& self) {
  auto x = self.to(ScalarType::Float64);
  PLUSS_CHECK(x.dim() == 1 || x.dim() == 2, "corrcoef: expected a 1D or 2D input");
  const bool was_1d = x.dim() == 1;
  if (was_1d) x = x.unsqueeze(0);           // one variable -> [1, N]

  Series c = covariance_impl(x, /*correction=*/1);          // [V, V]
  Series inv_sd = ops_internal::reciprocal(
      ops_internal::sqrt(diag_extract_2d(c, /*diagonal=*/0)));  // [V]
  Series corr = ops_internal::mul(
      ops_internal::mul(c, inv_sd.unsqueeze(1)), inv_sd.unsqueeze(0));
  corr = ops_internal::clamp_max(ops_internal::clamp_min(corr, -1.0), 1.0);

  return was_1d ? corr.reshape(std::vector<std::int64_t>{}) : corr;
}

// Straight-through backward for a special function whose derivative w.r.t. its
// tracked input `wrt` is `deriv(wrt)`. `deriv` runs under a NoGradGuard during
// the reverse pass, so it may call the (non-recording) special-function impls
// freely. Handles the case where `out` is a broadcast of `wrt` (two-arg funcs
// like igamma/zeta binding only their second operand).
static Series bind_special_grad_(
    Series out, const Series& wrt,
    std::function<Series(const Series&)> deriv, const char* op_name) {
  if (!series_tape::recording_enabled() || !series_tape::tracks_slope(wrt)) return out;
  if (!dtype_of(wrt.scalar_type()).is_floating()) return out;
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = op_name;
  link->parents = {wrt};
  link->propagate = [wrt, deriv = std::move(deriv), op_name](const Series& g) {
    series_tape::NoGradGuard ng;
    Series contrib = ops_internal::mul(
        g.to(wrt.platform()).to(ScalarType::Float64),
        deriv(wrt).to(wrt.platform()).to(ScalarType::Float64));
    contrib = reduce_broadcasted_like_(contrib, wrt, op_name);
    series_tape::accumulate_sensitivity(wrt, contrib.to(wrt.scalar_type()));
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}

static void check_special_inplace_dtype_(const Series& self, const char* op_name) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              std::string(op_name) + ": in-place form requires a floating-point Series");
}

// d/dx sinc(x) = (pi x cos(pi x) - sin(pi x)) / (pi x^2), with the removable
// singularity at 0 taking the limit value 0.
static double sinc_deriv_scalar(double x) {
  if (x == 0.0) return 0.0;
  const double px = kPi * x;
  return (px * std::cos(px) - std::sin(px)) / (kPi * x * x);
}

static double digamma_scalar(double x) {
  if (std::isnan(x)) return x;
  if (x == 0.0) return std::copysign(std::numeric_limits<double>::infinity(), -x);
  if (x < 0.0) {
    if (x == std::trunc(x)) return std::numeric_limits<double>::quiet_NaN();
    double integral = 0.0;
    const double r = std::modf(x, &integral);
    return digamma_scalar(1.0 - x) - kPi / std::tan(kPi * r);
  }
  double y = 0.0;
  while (x < 10.0) {
    y -= 1.0 / x;
    x += 1.0;
  }
  // psi(x) ~ ln x - 1/(2x) - sum B_{2k} / (2k x^{2k}).
  const double inv = 1.0 / x;
  const double inv2 = inv * inv;
  y += std::log(x) - 0.5 * inv -
       inv2 * (1.0 / 12.0 -
               inv2 * (1.0 / 120.0 -
                       inv2 * (1.0 / 252.0 -
                               inv2 * (1.0 / 240.0 - inv2 * (1.0 / 132.0)))));
  return y;
}

static Series digamma_impl(const Series& self) {
  return unary_special_impl_(self, [](double x) { return digamma_scalar(x); });
}

static Series erfinv_impl(const Series& self) {
  return unary_special_impl_(self, [](double x) { return erfinv_approx(x); });
}

// Elementwise integer factorial. Discrete integer-domain op in the same family
// as `factor`/`prime`/`is_prime`: integral/bool input, int64 output, not
// differentiable. Values 0..20 only (21! overflows int64). The result is a pure
// gather from a 21-entry table, so the elementwise step stays vectorized.
static Series factorial_impl(const Series& self) {
  PLUSS_CHECK(self.defined(), "factorial: input Series is undefined");
  const ScalarType st = self.scalar_type();
  PLUSS_CHECK(st == ScalarType::Bool || dtype_of(st).is_integral(),
              "factorial: expects integral or bool dtype, got " + std::string(dtype_of(st).name));
  const Platform plat = self.platform();
  const Series idx = self.to(Platform::CPU).to(ScalarType::Int64).contiguous();
  if (idx.numel() == 0) {
    Series empty = Series::empty(idx.sizes(), ScalarType::Int64, Platform::CPU);
    return plat == Platform::CPU ? empty : empty.to(plat);
  }

  static constexpr std::int64_t kMaxN = 20;  // 20! is the last factorial that fits int64
  const std::int64_t lo = idx.min().item<std::int64_t>();
  const std::int64_t hi = idx.max().item<std::int64_t>();
  PLUSS_CHECK(lo >= 0, "factorial: expects non-negative integer input");
  PLUSS_CHECK(hi <= kMaxN,
              "factorial: input too large; 21! overflows int64 (max supported input is 20)");

  static constexpr std::int64_t kTable[kMaxN + 1] = {
      1LL, 1LL, 2LL, 6LL, 24LL, 120LL, 720LL, 5040LL, 40320LL, 362880LL,
      3628800LL, 39916800LL, 479001600LL, 6227020800LL, 87178291200LL,
      1307674368000LL, 20922789888000LL, 355687428096000LL, 6402373705728000LL,
      121645100408832000LL, 2432902008176640000LL};
  Series lut = Series::empty({kMaxN + 1}, ScalarType::Int64, Platform::CPU);
  std::memcpy(lut.raw_mutable_data(), kTable, sizeof(kTable));

  Series out = lut.take(idx);  // gather: result shape == idx.sizes()
  return plat == Platform::CPU ? out : out.to(plat);
}

static double i0_scalar(double x) {
  const double ax = std::abs(x);
  if (ax < 3.75) {
    const double t = x / 3.75;
    const double t2 = t * t;
    return 1.0 + t2 * (3.5156229 + t2 * (3.0899424 + t2 * (1.2067492 +
           t2 * (0.2659732 + t2 * (0.0360768 + t2 * 0.0045813)))));
  }
  const double t = 3.75 / ax;
  return (std::exp(ax) / std::sqrt(ax)) *
         (0.39894228 + t * (0.01328592 + t * (0.00225319 + t * (-0.00157565 +
         t * (0.00916281 + t * (-0.02057706 + t * (0.02635537 +
         t * (-0.01647633 + t * 0.00392377))))))));
}

static double i1_scalar(double x) {
  const double ax = std::abs(x);
  double value;
  if (ax < 3.75) {
    const double y = (x / 3.75) * (x / 3.75);
    value = ax * (0.5 + y * (0.87890594 + y * (0.51498869 + y *
            (0.15084934 + y * (0.02658733 + y * (0.00301532 + y * 0.00032411))))));
  } else {
    const double y = 3.75 / ax;
    value = (std::exp(ax) / std::sqrt(ax)) *
            (0.39894228 + y * (-0.03988024 + y * (-0.00362018 + y *
            (0.00163801 + y * (-0.01031555 + y * (0.02282967 + y *
            (-0.02895312 + y * (0.01787654 - y * 0.00420059))))))));
  }
  return std::copysign(value, x);
}

static Series i0_impl(const Series& self) {
  return unary_special_impl_(self, [](double x) { return i0_scalar(x); });
}

static Series mvlgamma_impl(const Series& self, std::int64_t p) {
  PLUSS_CHECK(p >= 1, "mvlgamma: p must be >= 1");
  const double c = 0.25 * static_cast<double>(p) * static_cast<double>(p - 1) * std::log(kPi);
  // Domain: every gamma argument x - (j-1)/2 must be positive, i.e.
  // x > (p - 1) / 2. Outside it the multivariate gamma is undefined.
  const double lo = 0.5 * static_cast<double>(p - 1);
  return unary_special_impl_(self, [p, c, lo](double x) {
    if (!(x > lo)) return std::numeric_limits<double>::quiet_NaN();
    double acc = c;
    for (std::int64_t j = 1; j <= p; ++j) acc += std::lgamma(x + 0.5 * (1.0 - static_cast<double>(j)));
    return acc;
  });
}

static Series polygamma_impl(const Series& self, std::int64_t n) {
  PLUSS_CHECK(n >= 0, "polygamma: n must be >= 0");
  if (n == 0) return digamma_impl(self);
  // psi^(n)(x) = (-1)^(n+1) * n! * zeta(n+1, x) for n >= 1.
  double fact = 1.0;
  for (std::int64_t i = 2; i <= n; ++i) fact *= static_cast<double>(i);
  const double sign = ((n + 1) % 2 == 0) ? 1.0 : -1.0;
  const double order = static_cast<double>(n + 1);
  return unary_special_impl_(self, [order, fact, sign](double x) {
    return sign * fact * hurwitz_zeta_scalar(order, x);
  });
}

static Series frexp_impl(const Series& self) {
  return unary_f64_impl(self, [](double x) {
    int e = 0;
    return std::frexp(x, &e);
  });
}

// ldexp(x, e) = x * 2^e. Built as a multiply by a power-of-two scale so the
// backward carries d/dx = 2^e; the exponent is treated as a constant.
static Series ldexp_impl(const Series& self, double expv) {
  const auto dt = compat_real_dtype_for_(self);
  const double scale = std::exp2(expv);
  return ops_internal::mul(self.to(dt), scalar_like(self.to(dt), scale));
}

static Series ldexp_impl(const Series& self, const Series& exponents) {
  const auto dt = compat_real_dtype_for_(self);
  const Series scale = ops_internal::exp2(exponents.to(dt));
  return ops_internal::mul(self.to(dt), scale);
}

static Series prelu_impl(const Series& self, const Series& weight) {
  auto x = self.to(ScalarType::Float64);
  auto w = weight.to(ScalarType::Float64);
  auto zero = scalar_like(x, 0.0);
  auto pos = ops_internal::gt(x, zero);
  auto neg = ops_internal::mul(w, x);
  Series out = ops_internal::where(pos, x, neg);
  // Keep a floating result for integral/bool input; a learnable fractional
  // slope on negatives is not representable as an integer.
  return dtype_of(self.scalar_type()).is_floating() ? out.to(self.scalar_type()) : out;
}

static Series prelu_impl(const Series& self, double weight) {
  // Build the slope in a real dtype so an integral `self` does not truncate it.
  const ScalarType wdt = dtype_of(self.scalar_type()).is_floating()
                             ? self.scalar_type()
                             : ScalarType::Float64;
  return prelu_impl(self, Series::full({1}, weight, wdt).to(self.platform()));
}

// renorm(p, dim, maxnorm): rescale every sub-tensor select(dim, i) whose p-norm
// exceeds maxnorm so that it lands exactly on maxnorm; leave the rest untouched.
// The sub-tensor's norm is taken over *all* axes except `dim`, so the scale is
// broadcast back with shape 1 everywhere but `dim`.
static Series renorm_impl(const Series& self, double p, std::int64_t dim, double maxnorm) {
  PLUSS_CHECK(p > 0.0, "renorm: p must be > 0");
  PLUSS_CHECK(maxnorm >= 0.0, "renorm: maxnorm must be >= 0");
  auto x = self.to(ScalarType::Float64);
  PLUSS_CHECK(x.dim() >= 2, "renorm: expected input with at least 2 dimensions");
  dim = wrap_dim(dim, x.dim(), "renorm");
  Series acc = ops_internal::pow(ops_internal::abs(x), scalar_like(x, p));
  for (std::int64_t d = 0; d < x.dim(); ++d) {
    if (d == dim) continue;
    acc = ops_internal::sum(acc, d, /*keepdim=*/true);
  }
  Series norm = ops_internal::pow(acc, scalar_like(acc, 1.0 / p));
  Series over = ops_internal::gt(norm, scalar_like(norm, maxnorm));
  Series factor = ops_internal::div(scalar_like(norm, maxnorm),
                                    ops_internal::add(norm, scalar_like(norm, 1e-7)));
  Series scale = ops_internal::where(over, factor, scalar_like(norm, 1.0));
  return ops_internal::mul(x, scale).to(self.scalar_type());
}

static Series multinomial_impl_cpu_(const Series& self, std::int64_t num_samples, bool replacement) {
  PLUSS_CHECK(is_floating_dtype(self.scalar_type()),
              "multinomial: probabilities must have a floating-point dtype");
  PLUSS_CHECK(self.dim() == 1 || self.dim() == 2,
              "multinomial: probability input must be 1-D or 2-D");
  PLUSS_CHECK(num_samples > 0, "multinomial: num_samples must be greater than zero");
  const std::int64_t rows = self.dim() == 1 ? 1 : self.size(0);
  const std::int64_t categories = self.size(-1);
  PLUSS_CHECK(categories > 0, "multinomial: each distribution must contain at least one category");
  PLUSS_CHECK(replacement || num_samples <= categories,
              "multinomial: cannot sample more entries than categories without replacement");
  auto p = self.to(Platform::CPU).to(ScalarType::Float64).contiguous();
  const auto* pp = reinterpret_cast<const double*>(p.raw_data());
  std::vector<std::int64_t> out_shape = self.dim() == 1
      ? std::vector<std::int64_t>{num_samples}
      : std::vector<std::int64_t>{rows, num_samples};
  auto out = Series::empty(out_shape, ScalarType::Int64);
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  std::mt19937_64 gen(pluss::tils::seed_or_next(0));
  for (std::int64_t row = 0; row < rows; ++row) {
    std::vector<double> probs(static_cast<std::size_t>(categories));
    double total = 0.0;
    std::int64_t positive = 0;
    for (std::int64_t col = 0; col < categories; ++col) {
      const double value = pp[row * categories + col];
      PLUSS_CHECK(std::isfinite(value) && value >= 0.0,
                  "multinomial: probabilities must be finite and nonnegative");
      probs[static_cast<std::size_t>(col)] = value;
      total += value;
      positive += value > 0.0 ? 1 : 0;
    }
    PLUSS_CHECK(total > 0.0, "multinomial: each probability row must have a positive sum");
    PLUSS_CHECK(replacement || num_samples <= positive,
                "multinomial: too few nonzero categories to sample without replacement");
    for (std::int64_t sample = 0; sample < num_samples; ++sample) {
      std::discrete_distribution<std::int64_t> dist(probs.begin(), probs.end());
      const auto index = dist(gen);
      op[row * num_samples + sample] = index;
      if (!replacement) probs[static_cast<std::size_t>(index)] = 0.0;
    }
  }
  return out;
}

static Series multinomial_impl(const Series& self, std::int64_t num_samples, bool replacement) {
  if (self.platform() == Platform::CUDA) {
    const bool native_enabled = native::cuda::native_stochastic_enabled();
    const bool native_strict = native::cuda::native_stochastic_strict();
    const bool route_supported =
        native::cuda::supports_multinomial_stochastic(self, num_samples, replacement);
    return cuda_stochastic_route_or_bridge_(
        "multinomial",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          return multinomial_impl_cpu_(self, num_samples, replacement).to(Platform::CUDA);
        });
  }
  Series out = multinomial_impl_cpu_(self, num_samples, replacement);
  return self.platform() == Platform::CPU ? out : out.to(self.platform());
}

static double igamma_p_scalar(double a, double x) {
  if (!(a > 0.0) || x < 0.0 || std::isnan(a) || std::isnan(x)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (x == 0.0) return 0.0;
  if (std::isinf(x)) return 1.0;
  if (std::isinf(a)) return 0.0;
  constexpr int itmax = 512;
  constexpr double eps = 1e-12;
  constexpr double fpmin = 1e-300;

  const double gln = std::lgamma(a);
  if (x < a + 1.0) {
    double ap = a;
    double del = 1.0 / a;
    double sum = del;
    for (int n = 1; n <= itmax; ++n) {
      ap += 1.0;
      del *= x / ap;
      sum += del;
      if (std::abs(del) < std::abs(sum) * eps) {
        return sum * std::exp(-x + a * std::log(x) - gln);
      }
    }
    return sum * std::exp(-x + a * std::log(x) - gln);
  }

  double b = x + 1.0 - a;
  double c = 1.0 / fpmin;
  double d = 1.0 / b;
  double h = d;
  for (int i = 1; i <= itmax; ++i) {
    const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
    b += 2.0;
    d = an * d + b;
    if (std::abs(d) < fpmin) d = fpmin;
    c = b + an / c;
    if (std::abs(c) < fpmin) c = fpmin;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < eps) break;
  }
  return 1.0 - std::exp(-x + a * std::log(x) - gln) * h;
}

static Series igamma_impl(const Series& a, const Series& x, bool complement) {
  const auto promoted = promote_types(a.scalar_type(), x.scalar_type());
  const auto dt = (a.platform() == Platform::MPS || x.platform() == Platform::MPS)
                      ? ScalarType::Float32
                      : (is_floating_dtype(promoted) ? promoted : ScalarType::Float64);
  auto aa = a.to(dt);
  auto xx = x.to(dt);
  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), xx.sizes());
  auto abb = expand_impl(aa, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto xbb = expand_impl(xx, std::span<const std::int64_t>(shape.data(), shape.size())).contiguous();
  auto out = Series::empty(shape, dt, aa.platform());
  const auto n = out.numel();
  for (std::int64_t i = 0; i < n; ++i) {
    const double p = igamma_p_scalar(read_real_(abb, i), read_real_(xbb, i));
    write_real_(out, i, complement ? (1.0 - p) : p);
  }
  return out;
}

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    std::int64_t storage_offset_elems) {
  PLUSS_CHECK(sizes.size() == strides.size(), "as_strided: sizes/strides rank mismatch");
  PLUSS_CHECK(storage_offset_elems >= 0, "as_strided: storage_offset must be >= 0");
  std::vector<std::int64_t> out_sizes(sizes.begin(), sizes.end());
  std::vector<std::int64_t> out_strides(strides.begin(), strides.end());
  for (const auto size : out_sizes) PLUSS_CHECK(size >= 0, "as_strided: sizes must be non-negative");
  for (const auto stride : out_strides) PLUSS_CHECK(stride >= 0, "as_strided: negative strides are unsupported");

  const auto capacity = static_cast<std::int64_t>(
      self.storage().nbytes() / static_cast<std::size_t>(std::max<std::int64_t>(1, self.itemsize())));
  bool empty = false;
  std::int64_t max_offset = storage_offset_elems;
  for (std::size_t dim = 0; dim < out_sizes.size(); ++dim) {
    if (out_sizes[dim] == 0) {
      empty = true;
      continue;
    }
    const auto extent = out_sizes[dim] - 1;
    PLUSS_CHECK(out_strides[dim] == 0 ||
                    extent <= (std::numeric_limits<std::int64_t>::max() - max_offset) /
                        out_strides[dim],
                "as_strided: requested view offset overflows int64");
    max_offset += extent * out_strides[dim];
  }
  PLUSS_CHECK(empty ? storage_offset_elems <= capacity : max_offset < capacity,
              "as_strided: requested view exceeds storage bounds");

  // Unlike external interop views, as_strided intentionally permits overlap.
  Series out = ops_internal::SeriesFactoryAccess::wrap(
      pluss::msharp::make_intrusive<SeriesImpl>(
          self.storage(), storage_offset_elems, out_sizes, out_strides,
          self.scalar_type(), self.platform()));

  if (!series_tape::recording_enabled()) return out;
  if (!series_tape::tracks_slope(self)) {
    auto alias = std::make_shared<series_tape::SourceLink>();
    alias->op_name = "as_strided";
    alias->parents = {self};
    series_tape::bind_alias_link(out, std::move(alias));
    return out;
  }

  const std::vector<std::int64_t> input_sizes(self.sizes().begin(), self.sizes().end());
  const std::vector<std::int64_t> input_strides(self.strides().begin(), self.strides().end());
  const auto input_offset = self.storage_offset() / std::max<std::int64_t>(1, self.itemsize());
  auto link = std::make_shared<series_tape::SourceLink>();
  link->op_name = "as_strided";
  link->parents = {self};
  link->propagate = [self,
                     input_sizes,
                     input_strides,
                     input_offset,
                     out_sizes,
                     out_strides,
                     storage_offset_elems](const Series& out_sens) {
    PLUSS_CHECK(self.platform() == Platform::CPU,
                "as_strided: reverse-mode sensitivity currently supports CPU Series only");
    series_tape::NoGradGuard no_grad;

    auto logical_offsets = [](const std::vector<std::int64_t>& shape,
                              const std::vector<std::int64_t>& layout,
                              std::int64_t base) {
      std::int64_t count = 1;
      for (const auto size : shape) {
        if (size == 0) {
          count = 0;
          break;
        }
        count *= size;
      }
      std::vector<std::int64_t> offsets(static_cast<std::size_t>(count));
      for (std::int64_t linear = 0; linear < count; ++linear) {
        std::int64_t remainder = linear;
        std::int64_t offset = base;
        for (std::int64_t dim = static_cast<std::int64_t>(shape.size()) - 1; dim >= 0; --dim) {
          const auto size = shape[static_cast<std::size_t>(dim)];
          const auto coordinate = remainder % size;
          remainder /= size;
          offset += coordinate * layout[static_cast<std::size_t>(dim)];
        }
        offsets[static_cast<std::size_t>(linear)] = offset;
      }
      return offsets;
    };

    const auto output_offsets = logical_offsets(out_sizes, out_strides, storage_offset_elems);
    const auto parent_offsets = logical_offsets(input_sizes, input_strides, input_offset);
    std::unordered_map<std::int64_t, std::int64_t> parent_alias_count;
    for (const auto offset : parent_offsets) ++parent_alias_count[offset];

    Series grad;
    if (is_complex_dtype(self.scalar_type())) {
      const Series sens = out_sens.to(ScalarType::Complex128).contiguous();
      const auto* values = reinterpret_cast<const Complex128*>(sens.raw_data());
      std::unordered_map<std::int64_t, Complex128> storage_grad;
      for (std::size_t i = 0; i < output_offsets.size(); ++i) {
        storage_grad[output_offsets[i]] += values[i];
      }
      grad = Series::empty(input_sizes, ScalarType::Complex128);
      auto* destination = reinterpret_cast<Complex128*>(grad.raw_mutable_data());
      for (std::size_t i = 0; i < parent_offsets.size(); ++i) {
        const auto offset = parent_offsets[i];
        destination[i] = storage_grad[offset] /
            static_cast<double>(parent_alias_count[offset]);
      }
    } else {
      const Series sens = out_sens.to(ScalarType::Float64).contiguous();
      const auto* values = reinterpret_cast<const double*>(sens.raw_data());
      std::unordered_map<std::int64_t, double> storage_grad;
      for (std::size_t i = 0; i < output_offsets.size(); ++i) {
        storage_grad[output_offsets[i]] += values[i];
      }
      grad = Series::empty(input_sizes, ScalarType::Float64);
      auto* destination = reinterpret_cast<double*>(grad.raw_mutable_data());
      for (std::size_t i = 0; i < parent_offsets.size(); ++i) {
        const auto offset = parent_offsets[i];
        destination[i] = storage_grad[offset] /
            static_cast<double>(parent_alias_count[offset]);
      }
    }
    series_tape::accumulate_sensitivity(self, cast_like_(grad, self));
  };
  series_tape::bind_source_link(out, std::move(link));
  return out;
}

static Series as_strided_impl(
    const Series& self,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides) {
  const auto base_off = self.storage_offset() / std::max<std::int64_t>(1, self.itemsize());
  return as_strided_impl(self, sizes, strides, base_off);
}

static Series as_strided_scatter_impl(
    const Series& self,
    const Series& src,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    std::int64_t storage_offset_elems) {
  PLUSS_CHECK(sizes.size() == strides.size(), "as_strided_scatter: sizes/strides rank mismatch");
  PLUSS_CHECK(storage_offset_elems >= 0, "as_strided_scatter: storage_offset must be >= 0");
  ops_internal::require_same_platform(self, src, "as_strided_scatter");
  const bool track_self = series_tape::recording_enabled() && series_tape::tracks_slope(self);
  const bool track_src = series_tape::recording_enabled() && series_tape::tracks_slope(src);
  PLUSS_CHECK(!track_self || self.is_contiguous(),
              "as_strided_scatter: reverse-mode sensitivity requires a contiguous receiver");
  PLUSS_CHECK(self.platform() == Platform::CPU,
              "as_strided_scatter: native implementation currently supports CPU Series only");

  auto out = self.clone().contiguous();
  series_tape::set_tracks_slope(out, false);
  auto in = src.to(self.scalar_type()).contiguous();
  std::int64_t n = 1;
  std::int64_t max_offset = storage_offset_elems;
  for (std::size_t dim = 0; dim < sizes.size(); ++dim) {
    const auto s = sizes[dim];
    PLUSS_CHECK(s >= 0, "as_strided_scatter: invalid size");
    PLUSS_CHECK(strides[dim] >= 0, "as_strided_scatter: negative strides are unsupported");
    n *= s;
    if (s > 0) max_offset += (s - 1) * strides[dim];
  }
  PLUSS_CHECK(std::vector<std::int64_t>(src.sizes().begin(), src.sizes().end()) ==
                  std::vector<std::int64_t>(sizes.begin(), sizes.end()),
              "as_strided_scatter: src shape must equal the requested view shape");
  PLUSS_CHECK(n == 0 || max_offset < out.numel(),
              "as_strided_scatter: requested view exceeds receiver storage");
  const auto item = out.itemsize();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_mutable_data());
  const auto* sp = in.raw_data();
  std::vector<std::int64_t> idx(sizes.size(), 0);
  for (std::int64_t lin = 0; lin < n; ++lin) {
    std::int64_t rem = lin;
    for (std::int64_t d = static_cast<std::int64_t>(sizes.size()) - 1; d >= 0; --d) {
      const auto sd = sizes[(std::size_t)d];
      idx[(std::size_t)d] = rem % sd;
      rem /= sd;
    }
    std::int64_t off = storage_offset_elems;
    for (std::size_t d = 0; d < idx.size(); ++d) off += idx[d] * strides[d];
    std::memcpy(dst + off * item, sp + lin * item, static_cast<std::size_t>(item));
  }

  if (track_self || track_src) {
    const std::vector<std::int64_t> view_sizes(sizes.begin(), sizes.end());
    const std::vector<std::int64_t> view_strides(strides.begin(), strides.end());
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "as_strided_scatter";
    link->parents = {self, src};
    link->propagate = [self,
                       src,
                       track_self,
                       track_src,
                       view_sizes,
                       view_strides,
                       storage_offset_elems](const Series& out_sens) {
      series_tape::NoGradGuard no_grad;
      Series sens = cast_like_(out_sens, self).contiguous();
      if (track_self) {
        Series g_self = Series::zeros(self.sizes(), self.scalar_type()).to(self.platform());
        const auto itemsize = g_self.itemsize();
        auto* data = reinterpret_cast<std::byte*>(g_self.raw_mutable_data());
        const auto* source = sens.raw_data();
        std::int64_t count = 1;
        for (const auto size : view_sizes) count *= size;
        for (std::int64_t linear = 0; linear < count; ++linear) {
          std::int64_t remainder = linear;
          std::int64_t offset = storage_offset_elems;
          for (std::int64_t dim = static_cast<std::int64_t>(view_sizes.size()) - 1; dim >= 0; --dim) {
            const auto size = view_sizes[static_cast<std::size_t>(dim)];
            offset += (remainder % size) * view_strides[static_cast<std::size_t>(dim)];
            remainder /= size;
          }
          std::memcpy(data + offset * itemsize,
                      source + offset * itemsize,
                      static_cast<std::size_t>(itemsize));
        }
        series_tape::accumulate_sensitivity(self, cast_like_(g_self, self));
      }
      if (track_src) {
        Series g_src = as_strided_impl(
                           sens,
                           std::span<const std::int64_t>(view_sizes.data(), view_sizes.size()),
                           std::span<const std::int64_t>(view_strides.data(), view_strides.size()),
                           storage_offset_elems)
                           .contiguous();
        series_tape::accumulate_sensitivity(src, cast_like_(g_src, src));
      }
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series diagonal_scatter_impl(const Series& self, const Series& src, std::int64_t diagonal) {
  PLUSS_CHECK(self.dim() == 2, "diagonal_scatter: expected 2D input");
  auto out = self.clone().contiguous();
  auto in = src.to(self.scalar_type()).flatten(0, -1).contiguous();
  const auto rows = out.sizes()[0];
  const auto cols = out.sizes()[1];
  const auto row0 = diagonal < 0 ? -diagonal : 0;
  const auto col0 = diagonal > 0 ? diagonal : 0;
  const auto maxlen = std::max<std::int64_t>(0, std::min(rows - row0, cols - col0));
  const auto len = std::min(maxlen, in.numel());
  const auto item = out.itemsize();
  auto* dst = reinterpret_cast<std::byte*>(out.raw_mutable_data());
  const auto* sp = in.raw_data();
  for (std::int64_t i = 0; i < len; ++i) {
    const auto doff = ((row0 + i) * cols + (col0 + i)) * item;
    std::memcpy(dst + doff, sp + i * item, static_cast<std::size_t>(item));
  }
  const bool track_self = series_tape::recording_enabled() && series_tape::tracks_slope(self);
  const bool track_src = series_tape::recording_enabled() && series_tape::tracks_slope(src);
  if (track_self || track_src) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = "diagonal_scatter";
    link->parents = {self, src};
    link->propagate = [self, src, diagonal, len](const Series& out_sens) {
      Series sens = cast_like_(out_sens, self);
      if (series_tape::tracks_slope(self)) {
        Series base = sens.clone();
        Series zeros = Series::zeros({len}, self.scalar_type()).to(self.platform());
        Series g_self = diagonal_scatter_impl(base, zeros, diagonal);
        series_tape::accumulate_sensitivity(self, cast_like_(g_self, self));
      }
      if (series_tape::tracks_slope(src)) {
        Series diag_grad = diag_extract_2d(sens, diagonal);
        Series flat_grad = Series::zeros({src.numel()}, src.scalar_type()).to(src.platform());
        if (len > 0) {
          std::vector<std::int64_t> ids(static_cast<std::size_t>(len));
          std::iota(ids.begin(), ids.end(), static_cast<std::int64_t>(0));
          const Series index = i64_vector(ids).to(src.platform());
          auto put_fn = static_cast<Series (Series::*)(const Series&, const Series&, bool)>(&Series::put_);
          flat_grad = (flat_grad.*put_fn)(index, cast_like_(diag_grad, src), /*accumulate=*/false);
        }
        Series g_src = flat_grad.reshape(std::vector<std::int64_t>(src.sizes().begin(), src.sizes().end()));
        series_tape::accumulate_sensitivity(src, cast_like_(g_src, src));
      }
    };
    series_tape::bind_source_link(out, std::move(link));
  }
  return out;
}

static Series select_scatter_impl(const Series& self, const Series& src, std::int64_t dim, std::int64_t index) {
  dim = wrap_dim(dim, self.dim(), "select_scatter");
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (index < 0) index += dsz;
  PLUSS_CHECK(index >= 0 && index < dsz, "select_scatter: index out of range");
  PLUSS_CHECK(src.platform() == self.platform(), "select_scatter: self and src must be on the same platform");
  PLUSS_CHECK(src.scalar_type() == self.scalar_type(), "select_scatter: self and src must have the same dtype");
  std::vector<std::int64_t> expected(self.sizes().begin(), self.sizes().end());
  expected.erase(expected.begin() + dim);
  PLUSS_CHECK(src.sizes().size() == expected.size() &&
                  std::equal(src.sizes().begin(), src.sizes().end(), expected.begin()),
              "select_scatter: src shape must match self.select(dim, index)");
  auto x = src.unsqueeze(dim);
  auto idx = Series::full(x.sizes(), static_cast<double>(index), ScalarType::Int64).to(self.platform());
  return ops_internal::scatter(self, dim, idx, x);
}

static Series slice_scatter_impl(
    const Series& self,
    const Series& src,
    std::int64_t dim,
    std::int64_t start,
    std::int64_t end,
    std::int64_t step) {
  PLUSS_CHECK(step > 0, "slice_scatter: step must be > 0");
  dim = wrap_dim(dim, self.dim(), "slice_scatter");
  const auto dsz = self.sizes()[(std::size_t)dim];
  if (start < 0) start += dsz;
  if (end < 0) end += dsz;
  start = std::clamp<std::int64_t>(start, 0, dsz);
  end = std::clamp<std::int64_t>(end, 0, dsz);
  PLUSS_CHECK(end >= start, "slice_scatter: end must be >= start");
  std::vector<std::int64_t> ids;
  for (std::int64_t i = start; i < end; i += step) ids.push_back(i);

  PLUSS_CHECK(src.platform() == self.platform(), "slice_scatter: self and src must be on the same platform");
  PLUSS_CHECK(src.scalar_type() == self.scalar_type(), "slice_scatter: self and src must have the same dtype");
  std::vector<std::int64_t> expected(self.sizes().begin(), self.sizes().end());
  expected[(std::size_t)dim] = static_cast<std::int64_t>(ids.size());
  PLUSS_CHECK(src.sizes().size() == expected.size() &&
                  std::equal(src.sizes().begin(), src.sizes().end(), expected.begin()),
              "slice_scatter: src shape must match the selected slice of self");
  const auto& x = src;

  auto idx = Series::empty(x.sizes(), ScalarType::Int64).to(self.platform()).contiguous();
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_mutable_data());
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t d = 0; d < dim; ++d) outer *= x.sizes()[(std::size_t)d];
  for (std::int64_t d = dim + 1; d < x.dim(); ++d) inner *= x.sizes()[(std::size_t)d];
  const auto n = x.sizes()[(std::size_t)dim];
  for (std::int64_t o = 0; o < outer; ++o) {
    for (std::int64_t k = 0; k < n; ++k) {
      for (std::int64_t i = 0; i < inner; ++i) ip[(o * n + k) * inner + i] = ids[(std::size_t)k];
    }
  }
  return ops_internal::scatter(self, dim, idx, x);
}

static Series expand_axis_index_(
    const Series& self,
    std::int64_t& dim,
    const Series& index,
    const Series& source,
    const char* name) {
  PLUSS_CHECK(self.dim() > 0, std::string(name) + ": scalar receivers are not supported");
  dim = wrap_dim(dim, self.dim(), name);
  PLUSS_CHECK(index.dim() <= 1, std::string(name) + ": index must be a scalar or one-dimensional Series");
  PLUSS_CHECK(index.scalar_type() == ScalarType::Int32 || index.scalar_type() == ScalarType::Int64,
              std::string(name) + ": index dtype must be int32 or int64");
  PLUSS_CHECK(self.platform() == index.platform() && self.platform() == source.platform(),
              std::string(name) + ": self, index, and source must be on the same platform");
  PLUSS_CHECK(self.scalar_type() == source.scalar_type(),
              std::string(name) + ": self and source must have the same dtype");
  PLUSS_CHECK(source.dim() == self.dim(),
              std::string(name) + ": source and self must have the same rank");
  PLUSS_CHECK(source.sizes()[(std::size_t)dim] == index.numel(),
              std::string(name) + ": index length must equal source.size(dim)");
  for (std::int64_t axis = 0; axis < self.dim(); ++axis) {
    if (axis == dim) continue;
    PLUSS_CHECK(source.sizes()[(std::size_t)axis] == self.sizes()[(std::size_t)axis],
                std::string(name) + ": source shape must match self outside dim");
  }

  const Series normalized_index = index.to(ScalarType::Int64).contiguous();
  const Series host_index = normalized_index.to(Platform::CPU).contiguous();
  const auto* positions = reinterpret_cast<const std::int64_t*>(host_index.raw_data());
  const auto destination_size = self.sizes()[(std::size_t)dim];
  for (std::int64_t i = 0; i < host_index.numel(); ++i) {
    PLUSS_CHECK(positions[i] >= 0 && positions[i] < destination_size,
                std::string(name) + ": index value out of range");
  }

  std::vector<std::int64_t> index_shape((std::size_t)self.dim(), 1);
  index_shape[(std::size_t)dim] = index.numel();
  return normalized_index
      .reshape(index_shape)
      .expand(source.sizes(), true)
      .contiguous();
}

static Series index_add_impl_(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& source,
    double alpha) {
  const Series expanded_index = expand_axis_index_(self, dim, index, source, "index_add");
  const Series scaled = alpha == 1.0 ? source : mul_scalar(source, alpha).to(self.scalar_type());
  return ops_internal::scatter_add(self, dim, expanded_index, scaled);
}

static Series index_copy_impl_(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& source) {
  const Series expanded_index = expand_axis_index_(self, dim, index, source, "index_copy");
  return ops_internal::scatter(self, dim, expanded_index, source);
}

static Series index_fill_impl_(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    double value) {
  PLUSS_CHECK(self.dim() > 0, "index_fill: scalar receivers are not supported");
  dim = wrap_dim(dim, self.dim(), "index_fill");
  std::vector<std::int64_t> source_shape(self.sizes().begin(), self.sizes().end());
  source_shape[(std::size_t)dim] = index.numel();
  const Series source = Series::full(source_shape, value, self.scalar_type()).to(self.platform());
  const Series expanded_index = expand_axis_index_(self, dim, index, source, "index_fill");
  return ops_internal::scatter(self, dim, expanded_index, source);
}

static Series index_reduce_impl_(
    const Series& self,
    std::int64_t dim,
    const Series& index,
    const Series& source,
    std::string_view reduce,
    bool include_self) {
  PLUSS_CHECK(reduce == "prod" || reduce == "mean" || reduce == "amax" || reduce == "amin",
              "index_reduce: reduce must be one of 'prod', 'mean', 'amax', or 'amin'");
  const Series expanded_index = expand_axis_index_(self, dim, index, source, "index_reduce");
  return ops_internal::scatter_reduce(self, dim, expanded_index, source, std::string(reduce), include_self);
}

static std::string_view canonical_op(std::string_view op) {
  if (op == "absolute") return "abs";
  if (op == "arccos") return "acos";
  if (op == "arccosh") return "acosh";
  if (op == "arcsin") return "asin";
  if (op == "arcsinh") return "asinh";
  if (op == "arctan") return "atan";
  if (op == "arctan2") return "atan2";
  if (op == "arctanh") return "atanh";
  if (op == "divide") return "div";
  if (op == "true_divide") return "div";
  if (op == "multiply") return "mul";
  if (op == "subtract") return "sub";
  if (op == "clip") return "clamp";
  if (op == "greater") return "gt";
  if (op == "greater_equal") return "ge";
  if (op == "less") return "lt";
  if (op == "less_equal") return "le";
  if (op == "not_equal") return "ne";
  if (op == "negative") return "neg";
  if (op == "argwhere") return "nonzero";
  if (op == "nonzero_static") return "nonzero";
  if (op == "split_with_sizes") return "split";
  if (op == "unsafe_split_with_sizes") return "split";
  if (op == "conj_physical") return "conj";
  if (op == "_mkldnn_transpose") return "transpose";
  if (op == "_index_put_impl") return "index_put";
  if (op == "_resize_output") return "resize";
  if (op == "copy_sparse_to_sparse") return "copy";
  if (op == "_conj") return "conj";
  if (op == "_conj_physical") return "conj";
  if (op == "_to_dense") return "to_dense";
  return op;
}

static std::string_view canonical_inplace_op(std::string_view op) {
  if (op == "absolute_") return "abs_";
  if (op == "arccos_") return "acos_";
  if (op == "arccosh_") return "acosh_";
  if (op == "arcsin_") return "asin_";
  if (op == "arcsinh_") return "asinh_";
  if (op == "arctan_") return "atan_";
  if (op == "arctan2_") return "atan2_";
  if (op == "arctanh_") return "atanh_";
  if (op == "divide_") return "div_";
  if (op == "true_divide_") return "div_";
  if (op == "multiply_") return "mul_";
  if (op == "subtract_") return "sub_";
  if (op == "negative_") return "neg_";
  if (op == "greater_") return "gt_";
  if (op == "greater_equal_") return "ge_";
  if (op == "less_") return "lt_";
  if (op == "less_equal_") return "le_";
  if (op == "not_equal_") return "ne_";
  if (op == "clip_") return "clamp_";
  if (op == "_mkldnn_transpose_") return "transpose_";
  if (op == "_index_put_impl_") return "index_put_";
  if (op == "_resize_output_") return "resize_";
  if (op == "copy_sparse_to_sparse_") return "copy_";
  if (op == "_foreach_abs_") return "abs_";
  if (op == "_foreach_acos_") return "acos_";
  if (op == "_foreach_asin_") return "asin_";
  if (op == "_foreach_atan_") return "atan_";
  if (op == "_foreach_ceil_") return "ceil_";
  if (op == "_foreach_copy_") return "copy_";
  if (op == "_foreach_cos_") return "cos_";
  if (op == "_foreach_cosh_") return "cosh_";
  if (op == "_foreach_erf_") return "erf_";
  if (op == "_foreach_erfc_") return "erfc_";
  if (op == "_foreach_exp_") return "exp_";
  if (op == "_foreach_expm1_") return "expm1_";
  if (op == "_foreach_floor_") return "floor_";
  if (op == "_foreach_frac_") return "frac_";
  if (op == "_foreach_lgamma_") return "lgamma_";
  if (op == "_foreach_log10_") return "log10_";
  if (op == "_foreach_log1p_") return "log1p_";
  if (op == "_foreach_log2_") return "log2_";
  if (op == "_foreach_log_") return "log_";
  if (op == "_foreach_neg_") return "neg_";
  if (op == "_foreach_reciprocal_") return "reciprocal_";
  if (op == "_foreach_round_") return "round_";
  if (op == "_foreach_rsqrt_") return "rsqrt_";
  if (op == "_foreach_sigmoid_") return "sigmoid_";
  if (op == "_foreach_sign_") return "sign_";
  if (op == "_foreach_sin_") return "sin_";
  if (op == "_foreach_sinh_") return "sinh_";
  if (op == "_foreach_sqrt_") return "sqrt_";
  if (op == "_foreach_tan_") return "tan_";
  if (op == "_foreach_tanh_") return "tanh_";
  if (op == "_foreach_trunc_") return "trunc_";
  if (op == "_foreach_zero_") return "zero_";
  if (op == "_sobol_engine_ff_") return "random_";
  if (op == "_sobol_engine_initialize_state_") return "random_";
  if (op == "_sobol_engine_scramble_") return "random_";
  if (op == "_fill_mem_eff_dropout_mask_") return "dropout_";
  if (op == "range.out_") return "random_";
  if (op == "requires_grad_") return "tracks_slope_";
  return op;
}

static bool inplace_op_mutates_value_(std::string_view op) {
  if (op == "_coalesced_" || op == "sparse_resize_" || op == "resize_as_sparse_" ||
      op == "detach_" || op == "clear_sensitivity" || op == "tracks_slope_" ||
      op == "rename_" || op == "set_" || op == "squeeze_" || op == "t_" ||
      op == "transpose_" || op == "swapaxes_" || op == "swapdims_" ||
      op == "unsqueeze_" || op == "resize_" || op == "resize_as_") {
    return false;
  }
  return true;
}

static bool inplace_op_has_autodiff_write_support_(std::string_view op) {
  return op == "copy_" || op == "put_" || op == "index_put_" ||
      op == "_amp_foreach_non_finite_check_and_unscale_" ||
      op == "_amp_update_scale_" ||
      op == "_fused_adagrad_" ||
      op == "_fused_adam_" ||
      op == "_fused_adamw_" ||
      op == "_fused_sgd_";
}

static bool inplace_functional_bridge_enabled_() noexcept {
  return true;
}

static bool inplace_op_has_functional_bridge_(std::string_view op) {
  if (op == "copy_" || op == "put_" || op == "index_put_") return false;
  if (op == "_amp_foreach_non_finite_check_and_unscale_" ||
      op == "_amp_update_scale_" ||
      op == "_fused_adagrad_" ||
      op == "_fused_adam_" ||
      op == "_fused_adamw_" ||
      op == "_fused_sgd_") {
    return false;
  }
  if (op == "sparse_resize_and_clear_") return false;
  if (!op.ends_with('_')) return false;
  return true;
}

static std::string functional_bridge_target_op_(std::string_view inplace_op) {
  PLUSS_CHECK(inplace_op_has_functional_bridge_(inplace_op),
              "functional bridge target requested for unsupported in-place op");
  return std::string(inplace_op.substr(0, inplace_op.size() - 1));
}

static void check_inplace_autograd_write_(std::string_view op, const Series& self) {
  if (!inplace_op_mutates_value_(op)) return;
  if (!series_tape::recording_enabled()) return;
  if (!series_tape::tracks_slope(self)) return;
  const bool indexed_write =
      op == "put_" || op == "index_put_" || op == "index_add_" ||
      op == "index_copy_" || op == "index_fill_" || op == "index_reduce_" ||
      op == "scatter_" || op == "scatter_add_" || op == "scatter_reduce_" ||
      op == "masked_scatter_";
  if (indexed_write && series_tape::is_leaf(self)) {
    throw_error(
        std::string(op) +
        ": a leaf Series that tracks slope cannot be mutated in-place while recording is enabled");
  }
  if (inplace_op_has_autodiff_write_support_(op)) return;
  if (inplace_functional_bridge_enabled_() &&
      inplace_op_has_functional_bridge_(op)) return;
  throw_error(
      std::string(op) +
      ": in-place mutation on autograd-tracked Series is disallowed; mutate a detached/separate Series instead");
}

class InplaceVersionGuard final {
public:
  InplaceVersionGuard(Series& self, bool enabled)
      : self_(&self), enabled_(enabled), uncaught_(std::uncaught_exceptions()) {}

  ~InplaceVersionGuard() {
    if (!enabled_ || self_ == nullptr) return;
    if (std::uncaught_exceptions() != uncaught_) return;
    if (!self_->defined()) return;
    self_->storage().bump_version();
  }

private:
  Series* self_{nullptr};
  bool enabled_{false};
  int uncaught_{0};
};

static bool is_outsourced_linalg_op(std::string_view op) {
  return op == "det" || op == "inverse" || op == "logdet" || op == "slogdet" ||
         op == "matrix_exp" || op == "matrix_power" || op == "cholesky" ||
         op == "cholesky_inverse" || op == "cholesky_solve" || op == "lu_solve" ||
         op == "triangular_solve" || op == "pinverse" || op == "qr" || op == "svd" ||
         op == "geqrf" || op == "orgqr" || op == "ormqr";
}

static void throw_outsourced_linalg_(std::string_view op) {
  throw_error(
      std::string(op) +
      ": Series method is intentionally outsourced. Use pluss::subsp::math::linalg::" +
      std::string(op) + "(...) instead.");
}

[[noreturn]] static void rethrow_dispatch_error_(std::string_view lane, std::string_view op, const std::exception& e) {
  throw_error(
      std::string("CodeM ") + std::string(lane) + " dispatch failed for '" + std::string(op) +
      "': " + e.what());
}

Series invoke_any(std::string_view op, const Series& self, const Args& args) {
  const std::string op_snapshot(op);
  try {
    op = canonical_op(op);
    if (is_outsourced_linalg_op(op)) {
      throw_outsourced_linalg_(op);
    }
    ops_internal::PlatformScope platform_scope(self.platform());
    validate_series_arg_platforms(op, self, args);

  if (op == "_backward" || op == "reverse") {
    std::optional<Series> seed;
    std::size_t index = 0;
    if (!args.empty() && std::holds_alternative<const Series*>(args[index].v)) {
      seed = as_series(args[index]);
      ++index;
    }

    series_tape::BackpropOptions backprop_opt{};
    if (index < args.size()) {
      backprop_opt.keep_source_links = as_bool(args[index]);
      ++index;
    }
    if (index < args.size()) {
      const bool accumulate_sensitivity = as_bool(args[index]);
      backprop_opt.reset_sensitivity_before_run = !accumulate_sensitivity;
      ++index;
    }
    if (index < args.size()) {
      backprop_opt.record_propagation = as_bool(args[index]);
      ++index;
    }
    if (index != args.size()) {
      throw_error(
          "_backward/reverse: expected signature reverse([seed], [keep_graph], "
          "[accumulate_sensitivity], [track_higher_order])");
    }
    series_tape::backpropagate(self, seed, backprop_opt);
    return self;
  }

  if (op == "stft") {
    // stft(n_fft, hop_length=-1, win_length=-1, window={}, center=true,
    //      pad_mode="reflect", normalized=false, onesided=true,
    //      return_complex=true)
    PLUSS_CHECK(args.size() == 9, "stft: expected 9 arguments from the typed wrapper");
    const Series none{};
    const Series& window =
        std::holds_alternative<const Series*>(args[3].v) ? as_series(args[3]) : none;
    return stft_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]), window,
                     as_bool(args[4]), as_string(args[5]), as_bool(args[6]),
                     as_bool(args[7]), as_bool(args[8]));
  }

  if (op == "istft") {
    // istft(n_fft, hop_length=-1, win_length=-1, window={}, center=true,
    //       normalized=false, onesided=true, length=-1)
    PLUSS_CHECK(args.size() == 8, "istft: expected 8 arguments from the typed wrapper");
    const Series none{};
    const Series& window =
        std::holds_alternative<const Series*>(args[3].v) ? as_series(args[3]) : none;
    return istft_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]), window,
                      as_bool(args[4]), as_bool(args[5]), as_bool(args[6]), as_i64(args[7]));
  }

  // 0-arg common (unary)
  if (args.empty()) {
    // `_fw_primal` on a tensor with no forward-mode tangent is its own primal;
    // returning the receiver matches that. `_addmm_activation` needs mat1/mat2
    // and is handled in the 5-arg section.
    if (op == "_fw_primal") return self;
    if (op == "_autocast_to_full_precision") {
      if (self.scalar_type() == ScalarType::Float16 || self.scalar_type() == ScalarType::BFloat16) {
        return self.to(ScalarType::Float32);
      }
      return self;
    }
    if (op == "_autocast_to_reduced_precision") {
      if (self.scalar_type() == ScalarType::Float64 || self.scalar_type() == ScalarType::Float32) {
        return self.to(ScalarType::Float16);
      }
      return self;
    }
    if (op == "_dimI" || op == "_dimV") return i64_scalar(0);
    if (op == "_nested_tensor_size") return i64_vector(self.sizes());
    if (op == "_nested_tensor_storage_offsets") {
      const std::array<std::int64_t, 1> off{{self.storage_offset() / std::max<std::int64_t>(1, self.itemsize())}};
      return i64_vector(std::span<const std::int64_t>(off.data(), off.size()));
    }
    if (op == "_nested_tensor_strides") return i64_vector(self.strides());
    if (op == "_sparse_mask_projection") return self;
    if (op == "chalf") return self.to(ScalarType::Complex64);

    // elementwise math
    if (op == "relu") return ops_internal::relu(self);
    if (op == "relu6") return ops_internal::clamp(self, 0.0, 6.0);
    if (op == "sigmoid") return ops_internal::sigmoid(self);
    if (op == "hardsigmoid") return hardsigmoid_impl_(self);
    if (op == "hardswish") return hardswish_impl_(self);
    if (op == "softplus") return ops_internal::softplus(self, /*beta=*/1.0, /*threshold=*/20.0);
    if (op == "mish") return mish_impl_(self);
    if (op == "silu") return ops_internal::silu(self);
    if (op == "selu") return selu_impl_(self);
    if (op == "hardtanh") return ops_internal::clamp(self, -1.0, 1.0);
    if (op == "celu") return celu_impl_(self, 1.0);
    if (op == "elu") return elu_impl_(self, 1.0, 1.0, 1.0);
    if (is_dropout_family_(op)) {
      return dropout_family_impl_(op, self, /*p=*/0.5, /*train=*/true);
    }
    if (op == "gelu") return ops_internal::gelu(self);

    if (op == "exp") return ops_internal::exp(self);
    if (op == "log") return ops_internal::log(self);
    if (op == "sqrt") return ops_internal::sqrt(self);
    if (op == "rsqrt") return ops_internal::rsqrt(self);

    if (op == "sin") return ops_internal::sin(self);
    if (op == "cos") return ops_internal::cos(self);
    if (op == "tan") return ops_internal::tan(self);
    if (op == "asin") return ops_internal::asin(self);
    if (op == "acos") return ops_internal::acos(self);
    if (op == "atan") return ops_internal::atan(self);
    if (op == "sinh") return ops_internal::sinh(self);
    if (op == "cosh") return ops_internal::cosh(self);
    if (op == "tanh") return ops_internal::tanh(self);
    if (op == "asinh") return ops_internal::asinh(self);
    if (op == "acosh") return ops_internal::acosh(self);
    if (op == "atanh") return ops_internal::atanh(self);

    if (op == "erf") return ops_internal::erf(self);
    if (op == "erfc") return ops_internal::erfc(self);
    if (op == "lgamma") return ops_internal::lgamma(self);
    if (op == "factorial") return factorial_impl(self);
    if (op == "log1p") return ops_internal::log1p(self);
    if (op == "expm1") return ops_internal::expm1(self);
    if (op == "neg") return ops_internal::neg(self);
    if (op == "conj") return ops_internal::conj(self);
    if (op == "real") return ops_internal::real(self);
    if (op == "imag") return ops_internal::imag(self);
    if (op == "angle") return ops_internal::angle(self);

    if (op == "log2") return ops_internal::log2(self);
    if (op == "log10") return ops_internal::log10(self);
    if (op == "exp2") return ops_internal::exp2(self);
    if (op == "cbrt") return ops_internal::cbrt(self);
    if (op == "square") return ops_internal::square(self);
    if (op == "cube") return ops_internal::pow(self, 3.0);
    if (op == "reciprocal") return ops_internal::reciprocal(self);

    if (op == "trunc") return ops_internal::trunc(self);
    if (op == "floor") return ops_internal::floor(self);
    if (op == "ceil") return ops_internal::ceil(self);
    if (op == "round") return ops_internal::round(self);

    // predicates
    if (op == "isnan") return ops_internal::isnan(self);
    if (op == "isinf") return ops_internal::isinf(self);
    if (op == "isfinite") return ops_internal::isfinite(self);
    if (op == "invalid") return ops_internal::invalid(self);
    if (op == "invalid_index") return ops_internal::invalid_index(self);
    if (op == "isposinf") return ops_internal::logical_and(ops_internal::isinf(self), ops_internal::gt(self, scalar_like(self, 0.0)));
    if (op == "isneginf") return ops_internal::logical_and(ops_internal::isinf(self), ops_internal::lt(self, scalar_like(self, 0.0)));
    if (op == "isreal") {
      if (!is_complex_dtype(self.scalar_type())) return Series::ones(self.sizes(), ScalarType::Bool);
      auto im = ops_internal::imag(self);
      return ops_internal::eq(im, scalar_like(im, 0.0));
    }
    if (op == "logical_not") return ops_internal::logical_not(self);
    if (op == "signbit") return ops_internal::signbit(self);

    if (op == "abs") return ops_internal::abs(self);
    if (op == "abs2") return abs2_impl(self);
    if (op == "sign" || op == "sgn") {
      if (is_complex_dtype(self.scalar_type())) {
        // sign() has no ordering to fall back on for complex numbers; sgn()
        // generalizes to the unit-modulus projection z / |z| (and 0 at z = 0).
        PLUSS_CHECK(op == "sgn",
                    "sign: not supported for complex dtypes; use sgn()");
        Series mag = ops_internal::abs(self);
        // denom = |z| everywhere, except 1 where |z| == 0 -- there z is also 0,
        // so z / 1 = 0 gives the documented sgn(0) = 0 without a branch.
        Series is_zero = ops_internal::eq(mag, scalar_like(mag, 0.0)).to(mag.scalar_type());
        Series denom = ops_internal::add(mag, is_zero);
        return ops_internal::div(self, denom.to(self.scalar_type()));
      }
      ScalarType work_dt = ScalarType::Float64;
      if (self.platform() == Platform::MPS) {
        work_dt = ScalarType::Float32;
      }
      Series pos = ops_internal::gt(self, scalar_like(self, 0.0)).to(work_dt);
      Series neg = ops_internal::lt(self, scalar_like(self, 0.0)).to(work_dt);
      Series out = ops_internal::sub(pos, neg);
      return (out.scalar_type() == self.scalar_type()) ? out : out.to(self.scalar_type());
    }

    if (op == "sum") return ops_internal::sum(self);
    if (op == "mean") return ops_internal::mean(self);
    if (op == "median") return median_flat_impl(self, /*ignore_nan=*/false);
    if (op == "nanmedian") return median_flat_impl(self, /*ignore_nan=*/true);
    if (op == "mode") return mode_flat_impl(self);
    if (op == "quantile") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/false);
    if (op == "nanquantile") return quantile_flat_impl(self, 0.5, /*ignore_nan=*/true);
    if (op == "nansum") return nan_sum_impl(self);
    if (op == "nanmean") return nan_mean_impl(self);
    if (op == "nanvar") return nan_var_impl(self);
    if (op == "nanstd") return nan_std_impl(self);
    if (op == "skewness") return skewness_impl(self);
    if (op == "kurtosis") return kurtosis_impl(self);
    if (op == "logsumexp") return ops_internal::logsumexp(self, /*dim=*/-1, /*keepdim=*/false);
    if (op == "prod") return ops_internal::prod(self);
    if (op == "amax" || op == "max") return ops_internal::amax(self);
    if (op == "amin" || op == "min") return ops_internal::amin(self);
    if (op == "all") return ops_internal::all(self);
    if (op == "any") return ops_internal::any(self);
    if (op == "argmax") return ops_internal::argmax(self);
    if (op == "argmin") return ops_internal::argmin(self);
    if (op == "count_nonzero") {
      auto nz = ops_internal::nonzero(self);
      Series out = Series::zeros({}, ScalarType::Int64);
      *reinterpret_cast<std::int64_t*>(out.raw_data()) = (nz.dim() > 0) ? nz.sizes()[0] : 0;
      return out;
    }
    if (op == "_nnz") {
      auto nz = ops_internal::nonzero(self);
      return i64_scalar((nz.dim() > 0) ? nz.sizes()[0] : 0);
    }
    if (op == "_is_all_true") return ops_internal::all(self);
    if (op == "_is_any_true") return ops_internal::any(self);
    if (op == "_is_zerotensor") return ops_internal::all(ops_internal::eq(self, scalar_like(self, 0.0)));
    if (op == "is_complex") return bool_scalar(is_complex_dtype(self.scalar_type()));
    if (op == "is_floating_point") return bool_scalar(is_floating_dtype(self.scalar_type()));
    if (op == "is_signed") return bool_scalar(is_signed_dtype(self.scalar_type()));
    if (op == "tracks_slope") return bool_scalar(series_tape::tracks_slope(self));
    if (op == "has_sensitivity") return bool_scalar(series_tape::sensitivity(self).has_value());
    if (op == "sensitivity") return series_tape::sensitivity_or_zeros(self);
    if (op == "is_leaf") return bool_scalar(series_tape::is_leaf(self));
    if (op == "is_conj" || op == "is_neg" || op == "is_inference" ||
        op == "is_distributed" || op == "is_pinned" || op == "is_coalesced" || op == "is_set_to") {
      return bool_scalar(false);
    }
    if (op == "output_nr") return i64_scalar(0);
    if (op == "_version") return i64_scalar(self.defined() ? self.storage().version() : 0);
    if (op == "size") return i64_vector(self.sizes());
    if (op == "stride") return i64_vector(self.strides());
    if (op == "item") {
      PLUSS_CHECK(self.numel() == 1, "item: expected tensor with exactly one element");
      return self.flatten(0, -1).select(0, 0);
    }
    if (op == "is_nonzero") {
      PLUSS_CHECK(self.numel() == 1, "is_nonzero: expected tensor with exactly one element");
      Series s = self.flatten(0, -1).to(ScalarType::Float64);
      const double v = *reinterpret_cast<const double*>(s.raw_data());
      return bool_scalar(v != 0.0);
    }
    if (op == "alias") return self;
    if (op == "data") return self;
    if (op == "values_view") return self;
    if (op == "values_copy") return self.clone();
    if (op == "detach") return series_tape::separate(self);
    if (op == "_lazy_clone") return self.clone();
    if (op == "_neg_view") return ops_internal::neg(self);
    if (op == "to_dense") return self.clone();
    if (op == "fix") return ops_internal::trunc(self);
    if (op == "frac") return ops_internal::sub(self, ops_internal::floor(self));
    if (op == "deg2rad" || op == "rad2deg") {
      // Degrees/radians conversion always produces non-integers; promote an
      // integral or boolean input so the constant is not truncated to 0.
      const Series x = dtype_of(self.scalar_type()).is_floating()
                           ? self
                           : self.to(compat_real_dtype_for_(self));
      return mul_scalar(x, op == "deg2rad" ? (kPi / 180.0) : (180.0 / kPi));
    }
    if (op == "positive") return self;
    if (op == "ravel") return self.flatten(0, -1);
    if (op == "mT" || op == "mH" || op == "matrix_H" || op == "adjoint" ||
        op == "numpy_T") {
      PLUSS_CHECK(self.dim() >= 2,
                  std::string(op) +
                      ": expected a series with at least 2 dimensions, got a " +
                      std::to_string(self.dim()) + "-D series");
      const Series swapped = self.transpose(-2, -1);
      return (op == "mH" || op == "matrix_H" || op == "adjoint")
                 ? ops_internal::conj(swapped)
                 : swapped;
    }
    if (op == "sort") return sort_values_only(self, /*dim=*/-1, /*descending=*/false, /*stable=*/false);
    if (op == "argsort") return argsort_values_only(self, /*dim=*/-1, /*descending=*/false, /*stable=*/false);
    if (op == "msort") return sort_values_only(self, /*dim=*/0, /*descending=*/false, /*stable=*/false);
    if (op == "tril") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/false);
    if (op == "triu") return tril_triu_impl(self, /*diagonal=*/0, /*upper=*/true);
    if (op == "flipud") {
      const std::array<std::int64_t, 1> dims{{0}};
      return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "flipud");
    }
    if (op == "fliplr") {
      PLUSS_CHECK(self.dim() >= 2, "fliplr: expected input with rank >= 2");
      const std::array<std::int64_t, 1> dims{{1}};
      return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "fliplr");
    }
    if (op == "trace") return ops_internal::sum(diag_extract_2d(self, /*diagonal=*/0));
    if (op == "diag") {
      if (self.dim() == 1) return diag_embed_from_1d(self, /*diagonal=*/0);
      return diag_extract_2d(self, /*diagonal=*/0);
    }
    if (op == "diagonal") return diag_extract_2d(self, /*diagonal=*/0);
    if (op == "diagflat") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "diag_embed") return diag_embed_from_1d(self, /*diagonal=*/0);
    if (op == "rot90") return rot90_impl(self, 1, 0, 1);
    if (op == "dct") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Dct);
    if (op == "idct") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Idct);
    if (op == "dst") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Dst);
    if (op == "idst") return trig_xform_impl(self, /*dim=*/-1, TrigXform::Idst);
    if (op == "hilbert") return hilbert_impl(self, /*dim=*/-1);
    if (op == "ediff1d") return ediff1d_impl(self);
    if (op == "movsum") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Sum);
    if (op == "movmean") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Mean);
    if (op == "movmax") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Max);
    if (op == "movmin") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Min);
    if (op == "movstd") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Std);
    if (op == "movvar") return mov_impl(self, /*window=*/3, /*dim=*/-1, MovKind::Var);
    if (op == "normr") return norm_along_impl(self, (self.dim() > 1) ? 1 : 0);
    if (op == "normc") return norm_along_impl(self, 0);
    if (op == "center") return centered_impl(self);
    if (op == "standardize") return standardize_impl(self);
    if (op == "pdf") return pdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "logpdf") return logpdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "logcdf") return logcdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "cdf") return cdf_impl(self, /*mean=*/0.0, /*std=*/1.0);
    if (op == "det") return pluss::subsp::linearalg::det(self);
    if (op == "inverse") return pluss::subsp::linearalg::inverse(self);
    if (op == "logdet") return pluss::subsp::linearalg::logdet(self);
    if (op == "slogdet") return pluss::subsp::linearalg::slogdet(self);
    if (op == "matrix_exp") return pluss::subsp::linearalg::matrix_exp(self);
    if (op == "matrix_power") return pluss::subsp::linearalg::matrix_power(self, 1);
    if (op == "cholesky") return pluss::subsp::linearalg::cholesky(self, /*upper=*/false);
    if (op == "cholesky_inverse") return pluss::subsp::linearalg::cholesky_inverse(self, /*upper=*/false);
    if (op == "cov") return covariance_impl(self);
    if (op == "corrcoef") return corrcoef_impl(self);
    if (op == "cummax") return (self.dim() == 0) ? self.clone() : cum_extrema_impl(self, 0, /*is_max=*/true);
    if (op == "cummin") return (self.dim() == 0) ? self.clone() : cum_extrema_impl(self, 0, /*is_max=*/false);
    if (op == "digamma")
      return bind_special_grad_(digamma_impl(self), self,
          [](const Series& x) { return polygamma_impl(x, 1); }, "digamma");
    if (op == "erfinv")
      return bind_special_grad_(erfinv_impl(self), self,
          [](const Series& x) {
            return ops_internal::mul(ops_internal::exp(ops_internal::square(erfinv_impl(x))),
                                     scalar_like(x, std::sqrt(kPi) / 2.0));
          }, "erfinv");
    if (op == "i0")
      return bind_special_grad_(i0_impl(self), self,
          [](const Series& x) {
            return unary_special_impl_(x, [](double v) { return i1_scalar(v); });
          }, "i0");
    if (op == "frexp") return frexp_impl(self);
    if (op == "mvlgamma")
      return bind_special_grad_(mvlgamma_impl(self, 1), self,
          [](const Series& x) { return digamma_impl(x); }, "mvlgamma");
    if (op == "polygamma")
      return bind_special_grad_(polygamma_impl(self, 1), self,
          [](const Series& x) { return polygamma_impl(x, 2); }, "polygamma");
    if (op == "multinomial") return multinomial_impl(self, 1, /*replacement=*/true);
    if (op == "histogram") return self.histc(/*bins=*/10, /*min=*/0.0, /*max=*/0.0);
    if (op == "pinverse") return pluss::subsp::linearalg::pinverse(self, 1e-12);
    if (op == "q_per_channel_axis" || op == "q_zero_point") return i64_scalar(0);
    if (op == "q_scale") return Series::full({}, 1.0, ScalarType::Float64);
    if (op == "qscheme") return i64_scalar(0);
    if (op == "q_per_channel_scales") return Series::ones({1}, ScalarType::Float64);
    if (op == "q_per_channel_zero_points") return Series::zeros({1}, ScalarType::Int64);
    if (op == "erfcinv")
      return bind_special_grad_(erfcinv_impl(self), self,
          [](const Series& y) {
            return ops_internal::mul(ops_internal::exp(ops_internal::square(erfcinv_impl(y))),
                                     scalar_like(y, -std::sqrt(kPi) / 2.0));
          }, "erfcinv");
    if (op == "airy") return airy_impl(self);
    if (op == "zeta") return zeta_impl(self, scalar_like(self, 1.0));
    if (op == "besselj") return bessel_impl(self, /*order=*/0.0, "j");
    if (op == "bessely") return bessel_impl(self, /*order=*/0.0, "y");
    if (op == "besseli") return bessel_impl(self, /*order=*/0.0, "i");
    if (op == "besselk") return bessel_impl(self, /*order=*/0.0, "k");
    if (op == "histcounts") return self.histc(/*bins=*/10, /*min=*/0.0, /*max=*/0.0);
    if (op == "sinc")
      return bind_special_grad_(sinc_impl(self), self,
          [](const Series& x) { return unary_f64_impl(x, sinc_deriv_scalar); }, "sinc");
    if (op == "hardshrink") return hardshrink_impl(self, 0.5);
    if (op == "aminmax") return aminmax_impl(self);
    if (op == "logcumsumexp") return logcumsumexp_impl(self, -1);
    if (op == "unwrap") return unwrap_impl(self, kPi, -1);
    if (op == "cplxpair") return cplxpair_impl(self);
    if (op == "fftshift") return shift_impl(self, {}, /*inverse=*/false);
    if (op == "ifftshift") return shift_impl(self, {}, /*inverse=*/true);
    if (op == "unbind") return self.select(0, 0);
    if (op == "align_to") return self;
    if (op == "bernoulli") return bernoulli_sample_from_probs(self);
    if (op == "t") {
      if (self.dim() != 2) return self;
      return self.transpose(0, 1);
    }

    if (op == "contiguous") return self.contiguous();
    if (op == "clone") return self.clone();
    if (op == "dequantize" || op == "int_repr" || op == "coalesce" || op == "pin_memory" ||
        op == "record_stream" || op == "resolve_conj" || op == "resolve_neg") return self;
    if (op == "to_sparse" || op == "_to_sparse" ||
        op == "to_sparse_csr" || op == "_to_sparse_csr" ||
        op == "to_sparse_csc" || op == "_to_sparse_csc" ||
        op == "to_sparse_bsr" || op == "_to_sparse_bsr" ||
        op == "to_sparse_bsc" || op == "_to_sparse_bsc" ||
        op == "to_mkldnn" || op == "to_padded_tensor" ||
        op == "rename" || op == "refine_names" ||
        op == "set_data" || op == "rename_") return self;
    if (op == "retain_grad") {
      series_tape::set_keep_sensitivity(self, true);
      return self;
    }
    if (op == "retains_grad") return bool_scalar(series_tape::keeps_sensitivity(self));
    if (op == "sparse_mask") return self;
    if (op == "smm") return self;
    if (op == "sspaddmm") return self;
    if (op == "_indices" || op == "indices" ||
        op == "crow_indices" || op == "ccol_indices" || op == "col_indices" || op == "row_indices") return self.nonzero();
    if (op == "dense_dim" || op == "sparse_dim") return i64_scalar(0);
    if (op == "squeeze") {
      if (args.empty()) return ops_internal::squeeze(self);
      if (args.size() == 1) return ops_internal::squeeze(self, as_i64(args[0]));
      PLUSS_CHECK(false, "squeeze: expected 0 or 1 args");
    }

    if (op == "unique") {
      if (args.empty()) return ops_internal::unique(self, true);
      if (args.size() == 1) {
        if (std::holds_alternative<bool>(args[0].v)) {
          return ops_internal::unique(self, as_bool(args[0]));
        }
        return ops_internal::unique(self, as_i64(args[0]), /*sorted=*/true);
      }
      if (args.size() == 2) {
        return ops_internal::unique(self, as_i64(args[0]), as_bool(args[1]));
      }
      PLUSS_CHECK(false, "unique: expected 0, 1, or 2 args");
    }

    // bitwise unary
    if (op == "bitwise_not") {
      PLUSS_CHECK(args.empty(), "bitwise_not: expected 0 args");
      return ops_internal::bitwise_not(self);
    }
    if (op == "nonzero") return ops_internal::nonzero(self);

    if (op == "nan_to_num") {
      double posinf = 0.0, neginf = 0.0;
      nan_to_num_default_bounds_(self.scalar_type(), posinf, neginf);
      return ops_internal::nan_to_num(self, /*nan=*/0.0, posinf, neginf);
    }

    if (op == "bitwise_and") {
      PLUSS_CHECK(args.size() == 1, "bitwise_and: expected 1 arg");
      return ops_internal::bitwise_and(self, as_series(args[0]));
    }

    if (op == "bitwise_or") {
      PLUSS_CHECK(args.size() == 1, "bitwise_or: expected 1 arg");
      return ops_internal::bitwise_or(self, as_series(args[0]));
    }

    if (op == "bitwise_xor") {
      PLUSS_CHECK(args.size() == 1, "bitwise_xor: expected 1 arg");
      return ops_internal::bitwise_xor(self, as_series(args[0]));
    }

    return not_impl(op, args, self);
  }

  // 1-arg
  if (args.size() == 1) {
    const auto& a0 = args[0].v;

    // scalar
    if (is_numeric_scalar_arg(args[0])) {
      const double s = as_double(args[0]);
      if (op == "mul") return mul_scalar(self, s);
      if (op == "add") return add_scalar(self, s);
      if (op == "sub") return sub_scalar(self, s);
      if (op == "div") return div_scalar(self, s);
      if (op == "float_power") return float_power_impl_(self, scalar_like(self, s));
      if (op == "floor_divide") {
        // Weak-scalar rule: an integer-valued scalar against an integral series
        // keeps the result integral; a fractional scalar makes it floating.
        const bool keep_int = !is_floating_dtype(self.scalar_type()) &&
                              !is_complex_dtype(self.scalar_type()) && s == std::trunc(s);
        return floor_divide_impl_(
            self, keep_int ? scalar_like(self, s) : Series::full({}, s, ScalarType::Float64));
      }
      if (op == "pow") return pow_scalar(self, s);
      if (op == "norm") return self.norm(s);
      if (op == "copysign") {
        const double sign = std::signbit(s) ? -1.0 : 1.0;
        return mul_scalar(ops_internal::abs(self), sign);
      }
      if (op == "logaddexp") return ops_internal::logaddexp(self, scalar_like(self, s));
      if (op == "logaddexp2") return ops_internal::logaddexp2(self, scalar_like(self, s));
      if (op == "hypot") return ops_internal::hypot(self, scalar_like(self, s));
      if (op == "xlogy") return ops_internal::xlogy(self, scalar_like(self, s));
      if (op == "clamp_min") return ops_internal::clamp_min(self, s);
      if (op == "clamp_max") return ops_internal::clamp_max(self, s);
      if (op == "maximum") return ops_internal::maximum(self, scalar_like(self, s));
      if (op == "minimum") return ops_internal::minimum(self, scalar_like(self, s));
      if (op == "fmod") return ops_internal::fmod(self, scalar_like(self, s));
      if (op == "remainder") return ops_internal::remainder(self, scalar_like(self, s));
      if (op == "atan2") return ops_internal::atan2(self, scalar_like(self, s));
      if (op == "eq") return ops_internal::eq(self, scalar_like(self, s));
      if (op == "ne") return ops_internal::ne(self, scalar_like(self, s));
      if (op == "lt") return ops_internal::lt(self, scalar_like(self, s));
      if (op == "le") return ops_internal::le(self, scalar_like(self, s));
      if (op == "gt") return ops_internal::gt(self, scalar_like(self, s));
      if (op == "ge") return ops_internal::ge(self, scalar_like(self, s));
      if (op == "bincount") return ops_internal::bincount(self, (std::int64_t)s);
      if (op == "count_nonzero") {
        const std::int64_t nd = self.dim();
        std::int64_t d = (std::int64_t)s;
        if (d < 0) d += nd;
        PLUSS_CHECK(d >= 0 && d < nd, "count_nonzero: dim out of range");
        return ops_internal::sum(ops_internal::ne(self, scalar_like(self, 0.0)).to(ScalarType::Int64),
                                 d, /*keepdim=*/false);
      }
      if (op == "cov") return covariance_impl(self, (std::int64_t)s);
      if (op == "beta") return beta_impl(self, scalar_like(self, s));
      if (op == "betaln") return betaln_impl(self, scalar_like(self, s));
      if (op == "nextafter") return nextafter_impl(self, s);
      if (op == "logit") return logit_impl(self, s);
      if (op == "hardshrink") return hardshrink_impl(self, s);
      if (op == "leaky_relu") return ops_internal::leaky_relu(self, s);
      if (op == "celu") return celu_impl_(self, s);
      if (op == "elu") return elu_impl_(self, s, 1.0, 1.0);
      if (op == "heaviside") return heaviside_impl(self, scalar_like(self, s));
      if (op == "gcd") return gcd_impl(self, scalar_like(self, s));
      if (op == "lcm") return lcm_impl(self, scalar_like(self, s));
      if (op == "prelu") return prelu_impl(self, s);
      if (op == "ldexp") return ldexp_impl(self, s);
      if (op == "matrix_power") return pluss::subsp::linearalg::matrix_power(self, (std::int64_t)s);
      if (op == "mvlgamma") {
        const std::int64_t p = (std::int64_t)s;
        return bind_special_grad_(mvlgamma_impl(self, p), self,
            [p](const Series& x) {
              Series acc = digamma_impl(x);
              for (std::int64_t j = 2; j <= p; ++j)
                acc = ops_internal::add(
                    acc, digamma_impl(ops_internal::add(
                             x, scalar_like(x, 0.5 * (1.0 - (double)j)))));
              return acc;
            }, "mvlgamma");
      }
      if (op == "polygamma") {
        const std::int64_t n = (std::int64_t)s;
        return bind_special_grad_(polygamma_impl(self, n), self,
            [n](const Series& x) { return polygamma_impl(x, n + 1); }, "polygamma");
      }
      if (op == "multinomial") return multinomial_impl(self, (std::int64_t)s, /*replacement=*/false);
      if (op == "cummax") return cum_extrema_impl(self, (std::int64_t)s, /*is_max=*/true);
      if (op == "cummin") return cum_extrema_impl(self, (std::int64_t)s, /*is_max=*/false);
      if (op == "renorm") return renorm_impl(self, s, /*dim=*/0, /*maxnorm=*/1.0);
      if (op == "igamma") return igamma_impl(self, scalar_like(self, s), /*complement=*/false);
      if (op == "igammac") return igamma_impl(self, scalar_like(self, s), /*complement=*/true);
      if (op == "bitwise_left_shift") return bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/true);
      if (op == "bitwise_right_shift") return bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/false);
      if (op == "argsort") return argsort_values_only(self, (std::int64_t)s, /*descending=*/false, /*stable=*/false);
      if (op == "sort") return sort_values_only(self, (std::int64_t)s, /*descending=*/false, /*stable=*/false);
      if (op == "topk") return topk_values_only(self, (std::int64_t)s, /*dim=*/-1, /*largest=*/true, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, (std::int64_t)s, /*dim=*/-1, /*keepdim=*/false);
      if (op == "softmax") return ops_internal::softmax(self, (std::int64_t)s);
      if (op == "log_softmax") return ops_internal::log_softmax(self, (std::int64_t)s);
      if (op == "unsqueeze") return self.unsqueeze((std::int64_t)s);
      if (op == "squeeze") return ops_internal::squeeze(self, (std::int64_t)s);
      if (op == "split" || op == "unsafe_split") return split_first_by_size(self, (std::int64_t)s, /*dim=*/0);
      if (op == "chunk" || op == "unsafe_chunk") return chunk_first(self, (std::int64_t)s, /*dim=*/0);
      if (op == "tensor_split") return chunk_first(self, (std::int64_t)s, /*dim=*/0);
      if (op == "hsplit" || op == "vsplit" || op == "dsplit") return split_family_first(self, op, args[0]);
      if (op == "tril") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/false);
      if (op == "triu") return tril_triu_impl(self, (std::int64_t)s, /*upper=*/true);
      if (op == "flip") {
        const std::array<std::int64_t, 1> dims{{(std::int64_t)s}};
        return flip_impl(self, std::span<const std::int64_t>(dims.data(), dims.size()), "flip");
      }
      if (op == "diff") return diff_impl(self, (std::int64_t)s, /*dim=*/-1);
      if (op == "roll") return roll_flat_impl(self, (std::int64_t)s);
      if (op == "repeat_interleave") return repeat_interleave_impl(self.flatten(0, -1), (std::int64_t)s, 0);
      if (op == "median") return median_dim_impl(self, (std::int64_t)s, /*keepdim=*/false, /*ignore_nan=*/false);
      if (op == "nanmedian") return median_dim_impl(self, (std::int64_t)s, /*keepdim=*/false, /*ignore_nan=*/true);
      if (op == "mode") return mode_dim_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "skewness") return skewness_impl(self, (std::int64_t)s);
      if (op == "kurtosis") return kurtosis_impl(self, (std::int64_t)s);
      if (op == "quantile") return quantile_flat_impl(self, s, /*ignore_nan=*/false);
      if (op == "nanquantile") return quantile_flat_impl(self, s, /*ignore_nan=*/true);
      if (op == "dct") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Dct);
      if (op == "idct") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Idct);
      if (op == "dst") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Dst);
      if (op == "idst") return trig_xform_impl(self, (std::int64_t)s, TrigXform::Idst);
      if (op == "hilbert") return hilbert_impl(self, (std::int64_t)s);
      if (op == "movsum") return mov_impl(self, (std::int64_t)s, -1, MovKind::Sum);
      if (op == "movmean") return mov_impl(self, (std::int64_t)s, -1, MovKind::Mean);
      if (op == "movmax") return mov_impl(self, (std::int64_t)s, -1, MovKind::Max);
      if (op == "movmin") return mov_impl(self, (std::int64_t)s, -1, MovKind::Min);
      if (op == "movstd") return mov_impl(self, (std::int64_t)s, -1, MovKind::Std);
      if (op == "movvar") return mov_impl(self, (std::int64_t)s, -1, MovKind::Var);
      if (op == "unique") return ops_internal::unique(self, (std::int64_t)s, /*sorted=*/true);
      if (op == "histcounts") return self.histc((std::int64_t)s, 0.0, 0.0);
      if (op == "center") return centered_impl(self, (std::int64_t)s);
      if (op == "standardize") return standardize_impl(self, (std::int64_t)s);
      if (op == "nanvar") return nan_var_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nanstd") return nan_std_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "skewness") return skewness_impl(self, (std::int64_t)s);
      if (op == "kurtosis") return kurtosis_impl(self, (std::int64_t)s);
      if (op == "besselj") return bessel_impl(self, s, "j");
      if (op == "bessely") return bessel_impl(self, s, "y");
      if (op == "besseli") return bessel_impl(self, s, "i");
      if (op == "besselk") return bessel_impl(self, s, "k");
      if (op == "pdf") return pdf_impl(self, 0.0, s);
      if (op == "logpdf") return logpdf_impl(self, 0.0, s);
      if (op == "logcdf") return logcdf_impl(self, 0.0, s);
      if (op == "cdf") return cdf_impl(self, 0.0, s);
      if (op == "zeta") return zeta_impl(self, scalar_like(self, s));
      if (op == "unwrap") return unwrap_impl(self, s, -1);
      if (op == "rot90") return rot90_impl(self, (std::int64_t)s, 0, 1);
      if (op == "unbind") return self.select((std::int64_t)s, 0);
      if (op == "diag") {
        if (self.dim() == 1) return diag_embed_from_1d(self, (std::int64_t)s);
        return diag_extract_2d(self, (std::int64_t)s);
      }
      if (op == "diagonal") return diag_extract_2d(self, (std::int64_t)s);
      if (op == "diagflat") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "diag_embed") return diag_embed_from_1d(self, (std::int64_t)s);
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nansum") return nan_sum_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "nanmean") return nan_mean_impl(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "logsumexp") return ops_internal::logsumexp(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "logcumsumexp") return logcumsumexp_impl(self, (std::int64_t)s);
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amax" || op == "max") return ops_internal::amax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "amin" || op == "min") return ops_internal::amin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "all") return ops_internal::all(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "any") return ops_internal::any(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)s, /*keepdim=*/false);
      if (op == "cumsum") return self.cumsum((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod") return self.cumprod((std::int64_t)s, /*keepdim=*/false);
      if (op == "bernoulli") return bernoulli_like(self, s);
      if (is_dropout_family_(op)) {
        return dropout_family_impl_(op, self, s, /*train=*/true);
      }
      if (op == "histogram") return self.histc((std::int64_t)s, 0.0, 0.0);
      return not_impl(op, args, self);
    }

    // bool
    if (std::holds_alternative<Arg::SeriesList>(a0)) {
      const auto& indices = as_series_list(args[0]);
      if (op == "index") {
        std::vector<ExtractSpec> specs;
        specs.reserve(indices.size());
        for (const auto& idx : indices) specs.emplace_back(idx);
        return self.index(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
      }
      return not_impl(op, args, self);
    }

    // bool
    if (std::holds_alternative<bool>(a0)) {
      const bool b = as_bool(args[0]);
      if (op == "gelu") return ops_internal::gelu(self, b);
      if (op == "rrelu") return rrelu_impl_(self, 0.125, 0.3333333333333333, b);
      if (op == "requires_grad") {
        series_tape::set_tracks_slope(self, b);
        return self;
      }
      if (is_dropout_family_(op)) {
        return dropout_family_impl_(op, self, /*p=*/0.5, b);
      }
      if (op == "retain_grad") {
        series_tape::set_keep_sensitivity(self, b);
        return self;
      }
      if (op == "unique") return self.unique(b);
      if (op == "cholesky") return pluss::subsp::linearalg::cholesky(self, b);
      if (op == "cholesky_inverse") return pluss::subsp::linearalg::cholesky_inverse(self, b);
      if (op == "multinomial") return multinomial_impl(self, /*num_samples=*/1, b);
      return not_impl(op, args, self);
    }

    // Series
    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "set_data" || op == "set_") {
        // Backward-compat shim:
        // some call sites use `x.set_data(y)` in a mutating style and ignore
        // the return value. Route this through an in-place assignment path.
        Series src = other;
        if (self.defined()) {
          if (src.platform() != self.platform()) src = src.to(self.platform(), src.scalar_type());
          if (src.scalar_type() != self.scalar_type()) src = src.to(self.scalar_type());
        }
        Series& self_mut = mut_(self);
        self_mut = src;
        return self_mut;
      }
      if (op == "kron") return kron_impl(self, other);
      if (op == "ldexp") return ldexp_impl(self, other);
      if (op == "prelu") return prelu_impl(self, other);
      if (op == "diagonal_scatter") return diagonal_scatter_impl(self, other, /*diagonal=*/0);
      if (op == "cholesky_solve") return pluss::subsp::linearalg::cholesky_solve(other, self, /*upper=*/false);
      if (op == "lu_solve") return pluss::subsp::linearalg::lu_solve(other, self);
      if (op == "triangular_solve") return pluss::subsp::linearalg::triangular_solve(other, self, /*upper=*/true, /*unitriangular=*/false);
      if (op == "igamma" || op == "igammac") {
        const bool complement = (op == "igammac");
        // d/dx of the regularized lower incomplete gamma P(a, x) is
        //   x^(a-1) e^(-x) / Gamma(a);  the upper form Q = 1 - P negates it.
        Series a_copy = self;
        return bind_special_grad_(
            igamma_impl(self, other, complement), other,
            [a_copy, complement](const Series& x) {
              Series d = ops_internal::exp(ops_internal::sub(
                  ops_internal::sub(
                      ops_internal::mul(ops_internal::sub(a_copy, scalar_like(a_copy, 1.0)),
                                        ops_internal::log(x)),
                      x),
                  ops_internal::lgamma(a_copy)));
              return complement ? ops_internal::neg(d) : d;
            }, op == "igammac" ? "igammac" : "igamma");
      }
      if (op == "add") return ops_internal::add(self, other);
      if (op == "sub") return ops_internal::sub(self, other);
      if (op == "mul") return ops_internal::mul(self, other);
      if (op == "div") return ops_internal::div(self, other);
      if (op == "pow") return ops_internal::pow(self, other);
      if (op == "floor_divide") return floor_divide_impl_(self, other);

      if (op == "eq") return ops_internal::eq(self, other);
      if (op == "ne") return ops_internal::ne(self, other);
      if (op == "lt") return ops_internal::lt(self, other);
      if (op == "le") return ops_internal::le(self, other);
      if (op == "gt") return ops_internal::gt(self, other);
      if (op == "ge") return ops_internal::ge(self, other);

      if (op == "logical_and") return ops_internal::logical_and(self, other);
      if (op == "logical_or") return ops_internal::logical_or(self, other);
      if (op == "logical_xor") return ops_internal::logical_xor(self, other);
      if (op == "bitwise_and") return ops_internal::bitwise_and(self, other);
      if (op == "bitwise_or") return ops_internal::bitwise_or(self, other);
      if (op == "bitwise_xor") return ops_internal::bitwise_xor(self, other);

      if (op == "maximum") return ops_internal::maximum(self, other);
      if (op == "minimum") return ops_internal::minimum(self, other);
      if (op == "clamp_min") return self.clamp_min(other);
      if (op == "clamp_max") return self.clamp_max(other);
      if (op == "nextafter") return nextafter_impl(self, other);
      if (op == "bitwise_left_shift") return bitshift_series_impl(self, other, /*left=*/true);
      if (op == "bitwise_right_shift") return bitshift_series_impl(self, other, /*left=*/false);
      if (op == "fmod") return ops_internal::fmod(self, other);
      if (op == "remainder") return ops_internal::remainder(self, other);
      if (op == "atan2") return ops_internal::atan2(self, other);
      if (op == "float_power") return float_power_impl_(self, other);
      if (op == "copysign") {
        Series mag = ops_internal::abs(self);
        Series pos = ops_internal::ge(other, scalar_like(other, 0.0));
        return ops_internal::where(pos, mag, ops_internal::neg(mag));
      }
      if (op == "logaddexp") return ops_internal::logaddexp(self, other);
      if (op == "logaddexp2") return ops_internal::logaddexp2(self, other);
      if (op == "hypot") return ops_internal::hypot(self, other);
      if (op == "xlogy") return ops_internal::xlogy(self, other);
      if (op == "fmax") return fmax_fmin_impl_(self, other, /*is_max=*/true);
      if (op == "fmin") return fmax_fmin_impl_(self, other, /*is_max=*/false);
      if (op == "max") return ops_internal::maximum(self, other);
      if (op == "min") return ops_internal::minimum(self, other);
      if (op == "equal") return ops_internal::all(ops_internal::eq(self, other));
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, 1e-5, 1e-8, false));
      if (op == "isclose") return isclose_impl(self, other, 1e-5, 1e-8, false);
      if (op == "is_same_size") return bool_scalar(self.sizes().size() == other.sizes().size() &&
                                                   std::equal(self.sizes().begin(), self.sizes().end(), other.sizes().begin()));
      if (op == "reshape_as" || op == "view_as") return self.reshape(other.sizes());
      if (op == "type_as") return self.to(other.scalar_type());
      if (op == "ger") return matmul_with_tape_(self.unsqueeze(1), other.unsqueeze(0));

      if (op == "matmul") return matmul_with_tape_(self, other);
      if (op == "mm") {
        PLUSS_CHECK(self.dim() == 2 && other.dim() == 2, "mm: expected two 2D Series");
        return matmul_with_tape_(self, other);
      }
      if (op == "mv") {
        PLUSS_CHECK(self.dim() == 2 && other.dim() == 1,
                    "mv: expected a 2D matrix and a 1D vector");
        return matmul_with_tape_(self, other);
      }
      if (op == "bmm") {
        check_bmm_contract_(self, other, "bmm");
        return matmul_with_tape_(self, other);
      }
      if (op == "index") return self.take(other);
      if (op == "take") return self.take(other);
      if (op == "take_along_dim") {
        auto flat_self = self.flatten(0, -1);
        auto flat_idx = other.flatten(0, -1).to(ScalarType::Int64);
        return flat_self.take(flat_idx);
      }
      if (op == "dot") {
        PLUSS_CHECK(self.dim() == 1 && other.dim() == 1, "dot: expected two 1D Series");
        PLUSS_CHECK(self.numel() == other.numel(), "dot: inputs must have the same length");
        return ops_internal::sum(ops_internal::mul(self, other));
      }
      if (op == "inner") {
        auto a1 = self.flatten(0, -1);
        auto b1 = other.flatten(0, -1);
        PLUSS_CHECK(a1.numel() == b1.numel(), "inner: flattened inputs must have same numel");
        return ops_internal::sum(ops_internal::mul(a1, b1));
      }
      if (op == "outer") {
        check_outer_contract_(self, other, "outer");
        return matmul_with_tape_(self.unsqueeze(1), other.unsqueeze(0));
      }
      if (op == "vdot") return vdot_impl(self, other);
      if (op == "cross") return cross_impl(self, other, -1);
      if (op == "dist")
        return ops_internal::sub(self, other).norm(2.0).reshape(std::vector<std::int64_t>{});
      if (op == "expand_as") return self.expand(other.sizes());
      if (op == "expand_like") return expand_impl(self, other.sizes());
      if (op == "repeat_like") {
        const std::vector<std::int64_t> reps = repeat_factors_like_impl_(self, other);
        if (reps.empty()) return self.clone();
        return repeat_impl(self, std::span<const std::int64_t>(reps.data(), reps.size()));
      }
      if (op == "align_as") return self;
      if (op == "heaviside") return heaviside_impl(self, other);
      if (op == "gcd") return gcd_impl(self, other);
      if (op == "lcm") return lcm_impl(self, other);
      if (op == "beta") return beta_impl(self, other);
      if (op == "betaln") return betaln_impl(self, other);
      if (op == "zeta") {
        // Hurwitz zeta zeta(s, q): d/dq = -s * zeta(s + 1, q). The exponent
        // slot has no closed-form routing and is left non-differentiable.
        Series s_copy = self;
        return bind_special_grad_(
            zeta_impl(self, other), other,
            [s_copy](const Series& q) {
              return ops_internal::mul(
                  ops_internal::neg(s_copy),
                  zeta_impl(ops_internal::add(s_copy, scalar_like(s_copy, 1.0)), q));
            }, "zeta");
      }
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, other, 2.0);
      if (op == "sparse_mask") return ops_internal::where(other.to(ScalarType::Bool), self, scalar_like(self, 0.0));
      if (op == "smm") return matmul_with_tape_(self, other);

      if (op == "masked_select") return ops_internal::masked_select(self, other);
      if (op == "gather") return self.gather(/*dim=*/0, other);
      // note: masked_scatter needs 2 args (mask, src)
      return not_impl(op, args, self);
    }

    // intlist
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "_reshape_alias") return self.reshape(sizes);
      if (op == "reshape") return self.reshape(sizes);
      if (op == "view") return self.view(sizes);
      if (op == "expand") return expand_impl(self, sizes);
      if (op == "broadcast_to") return expand_impl(self, sizes);
      if (op == "permute") return self.permute(sizes);
      if (op == "split" || op == "unsafe_split") {
        if (sizes.empty()) return self.clone();
        PLUSS_CHECK(sizes[0] >= 0, "split: split_sizes entries must be >= 0");
        const auto dsz = self.sizes().empty() ? 0 : self.sizes()[0];
        return select_range(self, /*dim=*/0, /*start=*/0, std::min<std::int64_t>(sizes[0], dsz));
      }
      if (op == "split_with_sizes" || op == "unsafe_split_with_sizes") {
        return split_first_with_sizes(self, sizes, /*dim=*/0, "split_with_sizes");
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, /*dim=*/0, "tensor_split");
      if (op == "hsplit" || op == "vsplit" || op == "dsplit") return split_family_first(self, op, args[0]);
      if (op == "flip") return flip_impl(self, sizes, "flip");
      if (op == "repeat" || op == "tile") return repeat_impl(self, sizes);
      if (op == "roll") {
        PLUSS_CHECK(sizes.size() == 1,
                    "roll: flattened roll accepts exactly one shift; provide dims for multiple shifts");
        return roll_flat_impl(self, sizes[0]);
      }
      if (op == "fftshift") return shift_impl(self, sizes, /*inverse=*/false);
      if (op == "ifftshift") return shift_impl(self, sizes, /*inverse=*/true);
      if (op == "new_empty") return Series::empty(sizes, self.scalar_type());
      if (op == "new_empty_strided") return Series::empty(sizes, self.scalar_type());
      if (op == "new_zeros") return Series::zeros(sizes, self.scalar_type());
      if (op == "new_ones") return Series::ones(sizes, self.scalar_type());
      if (op == "align_to") return self.permute(sizes);
      if (op == "sum_to_size") return sum_to_size_impl(self, sizes);
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 2-arg
  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;

    // scalar + scalar
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1])) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "add") return add_scalar(self, x * y);
      if (op == "sub") return sub_scalar(self, x * y);
      if (op == "clamp") return ops_internal::clamp(self, x, y);
      if (op == "hardtanh") return ops_internal::clamp(self, x, y);
      if (op == "threshold") return threshold_impl_(self, x, y);
      if (op == "rrelu") return rrelu_impl_(self, x, y, /*training=*/false);
      if (op == "softplus") return ops_internal::softplus(self, x, y);
      if (op == "nan_to_num") return ops_internal::nan_to_num(self, x, y, y);
      if (op == "lerp") return ops_internal::add(self, mul_scalar(ops_internal::sub(scalar_like(self, x), self), y));
      if (op == "leaky_relu") return ops_internal::leaky_relu(self, x);
      if (op == "squeeze") return ops_internal::squeeze(self, (std::int64_t)x);
      if (op == "unsqueeze") return self.unsqueeze((std::int64_t)x);
      if (op == "transpose") return self.transpose((std::int64_t)x, (std::int64_t)y);
      if (op == "swapdims" || op == "swapaxes")
        return self.transpose((std::int64_t)x, (std::int64_t)y);
      if (op == "movedim" || op == "moveaxis") {
        // Move axis `src` to position `dst`, shifting the rest — not a plain
        // two-axis swap (which is what `swapaxes` does).
        const std::int64_t nd = self.dim();
        const std::int64_t src = wrap_dim((std::int64_t)x, nd, "moveaxis");
        const std::int64_t dst = wrap_dim((std::int64_t)y, nd, "moveaxis");
        if (src == dst) return self;
        std::vector<std::int64_t> order;
        order.reserve((std::size_t)nd);
        for (std::int64_t i = 0; i < nd; ++i) {
          if (i != src) order.push_back(i);
        }
        order.insert(order.begin() + dst, src);
        return self.permute(order);
      }
      if (op == "sum") return ops_internal::sum(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "mean") return ops_internal::mean(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "prod") return ops_internal::prod(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amax") return ops_internal::amax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "amin") return ops_internal::amin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "all") return ops_internal::all(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "any") return ops_internal::any(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmax") return ops_internal::argmax(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argmin") return ops_internal::argmin(self, (std::int64_t)x, as_bool(args[1]));
      if (op == "argsort") return argsort_values_only(self, (std::int64_t)x, as_bool(args[1]), /*stable=*/false);
      if (op == "sort") return sort_values_only(self, (std::int64_t)x, /*descending=*/(y != 0.0), /*stable=*/false);
      if (op == "topk") return topk_values_only(self, (std::int64_t)x, (std::int64_t)y, /*largest=*/true, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, (std::int64_t)x, (std::int64_t)y, /*keepdim=*/false);
      if (op == "split" || op == "unsafe_split") return split_first_by_size(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "chunk" || op == "unsafe_chunk") return chunk_first(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "tensor_split") return tensor_split_first(self, args[0], (std::int64_t)y);
      if (op == "diff") return diff_impl(self, (std::int64_t)x, (std::int64_t)y);
      if (op == "flatten") return self.flatten((std::int64_t)x, (std::int64_t)y);
      if (op == "movsum") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Sum);
      if (op == "movmean") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Mean);
      if (op == "movmax") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Max);
      if (op == "movmin") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Min);
      if (op == "movstd") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Std);
      if (op == "movvar") return mov_impl(self, (std::int64_t)x, (std::int64_t)y, MovKind::Var);
      if (op == "pdf") return pdf_impl(self, x, y);
      if (op == "logpdf") return logpdf_impl(self, x, y);
      if (op == "logcdf") return logcdf_impl(self, x, y);
      if (op == "cdf") return cdf_impl(self, x, y);
      if (op == "unwrap") return unwrap_impl(self, x, (std::int64_t)y);
      if (op == "roll") {
        const std::array<std::int64_t, 1> shifts{{(std::int64_t)x}};
        const std::array<std::int64_t, 1> dims{{(std::int64_t)y}};
        return roll_impl(self,
                         std::span<const std::int64_t>(shifts.data(), shifts.size()),
                         std::span<const std::int64_t>(dims.data(), dims.size()));
      }
      if (op == "repeat_interleave") return repeat_interleave_impl(self, (std::int64_t)x, (std::int64_t)y);
      if (is_dropout_family_(op)) {
        return dropout_family_impl_(op, self, x, /*train=*/(y != 0.0));
      }
      return not_impl(op, args, self);
    }

    // dim + keepdim(bool)
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const double p = as_double(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (op == "multinomial") return multinomial_impl(self, dim, keepdim);
      if (is_dropout_family_(op)) {
        return dropout_family_impl_(op, self, p, keepdim);
      }
      if (op == "sum") return ops_internal::sum(self, dim, keepdim);
      if (op == "mean") return ops_internal::mean(self, dim, keepdim);
      if (op == "nansum") return nan_sum_impl(self, dim, keepdim);
      if (op == "nanmean") return nan_mean_impl(self, dim, keepdim);
      if (op == "nanvar") return nan_var_impl(self, dim, keepdim);
      if (op == "nanstd") return nan_std_impl(self, dim, keepdim);
      if (op == "logsumexp") return ops_internal::logsumexp(self, dim, keepdim);
      if (op == "prod") return ops_internal::prod(self, dim, keepdim);
      if (op == "amax" || op == "max") return ops_internal::amax(self, dim, keepdim);
      if (op == "amin" || op == "min") return ops_internal::amin(self, dim, keepdim);
      if (op == "all") return ops_internal::all(self, dim, keepdim);
      if (op == "any") return ops_internal::any(self, dim, keepdim);
      if (op == "argmax") return ops_internal::argmax(self, dim, keepdim);
      if (op == "argmin") return ops_internal::argmin(self, dim, keepdim);
      if (op == "unique") return ops_internal::unique(self, dim, keepdim);
      if (op == "argsort") return argsort_values_only(self, dim, keepdim, /*stable=*/false);
      if (op == "sort") return sort_values_only(self, dim, keepdim, /*stable=*/false);
      if (op == "cumsum") return self.cumsum(dim, keepdim);
      if (op == "cumprod") return self.cumprod(dim, keepdim);
      if (op == "center") return centered_impl(self, dim);
      if (op == "standardize") return standardize_impl(self, dim);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<bool>(a1)) {
      const Series& other = as_series(args[0]);
      const bool b = as_bool(args[1]);
      if (op == "cholesky_solve") return pluss::subsp::linearalg::cholesky_solve(other, self, b);
      if (op == "triangular_solve") return pluss::subsp::linearalg::triangular_solve(other, self, b, /*unitriangular=*/false);
      if (op == "lu_solve") return pluss::subsp::linearalg::lu_solve(other, self);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && is_numeric_scalar_arg(args[1])) {
      auto sizes = as_intlist(args[0]);
      if (op == "split" || op == "unsafe_split") {
        if (sizes.empty()) return self.clone();
        PLUSS_CHECK(sizes[0] >= 0, "split: split_sizes entries must be >= 0");
        const auto dim = as_i64(args[1]);
        return split_first_by_size(self, sizes[0], dim);
      }
      if (op == "split_with_sizes" || op == "unsafe_split_with_sizes") {
        return split_first_with_sizes(self, sizes, as_i64(args[1]), "split_with_sizes");
      }
      if (op == "tensor_split") return split_first_by_boundaries(self, sizes, as_i64(args[1]), "tensor_split");
      if (op == "new_full") return Series::full(sizes, as_double(args[1]), self.scalar_type());
      if (op == "roll") {
        const std::array<std::int64_t, 1> dim_one{{as_i64(args[1])}};
        return roll_impl(self, sizes, std::span<const std::int64_t>(dim_one.data(), dim_one.size()));
      }
      if (op == "fftshift") return shift_impl(self, sizes, /*inverse=*/false);
      if (op == "ifftshift") return shift_impl(self, sizes, /*inverse=*/true);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<Arg::IntList>(a1)) {
      const auto dim = as_i64(args[0]);
      auto sizes = as_intlist(args[1]);
      if (op == "unflatten") return unflatten_impl(self, dim, sizes);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<bool>(a1)) {
      auto sizes = as_intlist(args[0]);
      if (op == "expand") return expand_impl(self, sizes);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<Arg::IntList>(a1)) {
      auto shifts = as_intlist(args[0]);
      auto dims = as_intlist(args[1]);
      if (op == "as_strided" || op == "_reshape_alias") return as_strided_impl(self, shifts, dims);
      if (op == "new_empty_strided") {
        auto out = Series::empty(shifts, self.scalar_type());
        return Series::from_storage(out.storage(), /*storage_offset=*/0,
                                    std::vector<std::int64_t>(shifts.begin(), shifts.end()),
                                    std::vector<std::int64_t>(dims.begin(), dims.end()),
                                    self.scalar_type());
      }
      if (op == "roll") return roll_impl(self, shifts, dims);
      return not_impl(op, args, self);
    }

    // Series + scalar
    if (std::holds_alternative<const Series*>(a0) && is_numeric_scalar_arg(args[1])) {
      const Series& other = as_series(args[0]);
      const double s = as_double(args[1]);
      // Torch-style add/sub signatures: add(other, alpha), sub(other, alpha)
      if (op == "add") return ops_internal::add(self, mul_scalar(other, s));
      if (op == "sub") return ops_internal::sub(self, mul_scalar(other, s));
      if (op == "where") return self.where(other, s);
      if (op == "diagonal_scatter") return diagonal_scatter_impl(self, other, (std::int64_t)s);
      if (op == "cholesky_solve") return pluss::subsp::linearalg::cholesky_solve(other, self, s != 0.0);
      if (op == "triangular_solve") return pluss::subsp::linearalg::triangular_solve(other, self, s != 0.0, /*unitriangular=*/false);
      if (op == "lu_solve") return pluss::subsp::linearalg::lu_solve(other, self);
      if (op == "lerp") return ops_internal::add(self, mul_scalar(ops_internal::sub(other, self), s));
      if (op == "take_along_dim") return self.gather((std::int64_t)s, other);
      if (op == "masked_fill") return ops_internal::masked_fill(self, other, s);
      if (op == "beta") return beta_impl(self, scalar_like(self, s));
      if (op == "betaln") return betaln_impl(self, scalar_like(self, s));
      if (op == "zeta") return zeta_impl(self, scalar_like(self, s));
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, other, s);
      if (op == "dist")
        return ops_internal::sub(self, other).norm(s).reshape(std::vector<std::int64_t>{});
      if (op == "hardshrink_backward") return hardshrink_backward_impl(self, other, s);
      if (op == "cross") return cross_impl(self, other, (std::int64_t)s);
      return not_impl(op, args, self);
    }


// scalar + Series (dim, index/src)
if (is_numeric_scalar_arg(args[0]) &&
    std::holds_alternative<const Series*>(a1)) {
  const auto dim = as_i64(args[0]);
  const Series& other = as_series(args[1]);
  if (op == "index_put" || op == "put") {
    Series index = Series::full({1}, static_cast<double>(dim), ScalarType::Int64).to(self.platform(), ScalarType::Int64);
    return put_dispatch_copy_(self, index, other, /*accumulate=*/false);
  }
  if (op == "gather") return self.gather(dim, other);
  if (op == "take_along_dim") return self.gather(dim, other);
  if (op == "index_select") return index_select_dispatch_(self, dim, other);
  if (op == "scatter") return self;
  if (op == "scatter_add") return self;
  return not_impl(op, args, self);
}
// Series + Series
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& m = as_series(args[0]);
      const Series& s = as_series(args[1]);
      if (op == "where") return self.where(m, s);
      if (op == "masked_fill") return ops_internal::masked_fill(self, m, s);
      if (op == "cdist" || op == "pdist2") return cdist_impl(self, m, 2.0);
      if (op == "addmm") {
        check_mm_contract_(m, s, "addmm");
        return combine_beta_alpha(self, matmul_with_tape_(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "addmv") {
        check_mv_contract_(m, s, "addmv");
        return combine_beta_alpha(self, matmul_with_tape_(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "addr") {
        check_outer_contract_(m, s, "addr");
        return combine_beta_alpha(self, matmul_with_tape_(m.unsqueeze(1), s.unsqueeze(0)), /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "addbmm") {
        Series upd = matmul_with_tape_(m, s).sum(/*dim=*/0, /*keepdim=*/false);
        return combine_beta_alpha(self, upd, /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "baddbmm") {
        check_bmm_contract_(m, s, "baddbmm");
        return combine_beta_alpha(self, matmul_with_tape_(m, s), /*beta=*/1.0, /*alpha=*/1.0);
      }
      if (op == "lerp") return ops_internal::add(self, ops_internal::mul(ops_internal::sub(m, self), s));
      if (op == "masked_scatter") return self.masked_scatter(m, s);
      if (op == "where") return ops_internal::where(self, m, s);
      if (op == "index_put") return put_dispatch_copy_(self, m, s, /*accumulate=*/false);
      if (op == "put") {
        return put_dispatch_copy_(self, m, s, /*accumulate=*/false);
      }
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<Arg::SeriesList>(a0)) {
      const auto& indices = as_series_list(args[0]);
      if (op == "index") {
        std::vector<ExtractSpec> specs;
        specs.reserve(indices.size());
        for (const auto& idx : indices) specs.emplace_back(idx);
        return self.index(pluss::msharp::RangeView::Of<ExtractSpec>(specs));
      }
      return not_impl(op, args, self);
    }

    return not_impl(op, args, self);
  }

  // 3-arg
  if (args.size() == 3) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<bool>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const bool descending = as_bool(args[1]);
      const bool stable = as_bool(args[2]);
      if (op == "argsort") return argsort_values_only(self, dim, descending, stable);
      if (op == "sort") return sort_values_only(self, dim, descending, stable);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto x = as_i64(args[0]);
      const auto y = as_i64(args[1]);
      const bool b = as_bool(args[2]);
      if (op == "rrelu") return rrelu_impl_(self, as_double(args[0]), as_double(args[1]), b);
      if (op == "topk") return topk_values_only(self, x, y, b, /*sorted=*/true);
      if (op == "kthvalue") return kthvalue_values_only(self, x, y, b);
      if (op == "norm") return self.norm(as_double(args[0]), y, b);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      if (op == "scatter") {
        return ops_internal::scatter(self, dim, index, src);
      }
      if (op == "scatter_add") {
        return ops_internal::scatter_add(self, dim, index, src);
      }
      if (op == "index_copy") {
        return index_copy_impl_(self, dim, index, as_series(args[2]));
      }
      if (op == "index_add") {
        return index_add_impl_(self, dim, index, as_series(args[2]), 1.0);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const double fill_value = as_double(args[2]);
      if (op == "index_fill") {
        return index_fill_impl_(self, dim, index, fill_value);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      if (op == "elu") return elu_impl_(self, as_double(args[0]), as_double(args[1]), as_double(args[2]));
      if (op == "renorm") return renorm_impl(self, as_double(args[0]), as_i64(args[1]), as_double(args[2]));
      if (op == "unfold") return unfold_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));
      if (op == "rot90") return rot90_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]));
      const auto diagonal = as_i64(args[0]);
      const auto dim1 = as_i64(args[1]);
      const auto dim2 = as_i64(args[2]);
      if (op == "diagonal") {
        PLUSS_CHECK(self.dim() == 2, "diagonal: currently implemented for 2D");
        auto d1 = wrap_dim(dim1, self.dim(), "diagonal");
        auto d2 = wrap_dim(dim2, self.dim(), "diagonal");
        PLUSS_CHECK(d1 != d2, "diagonal: dim1 and dim2 must differ");
        if (d1 == 0 && d2 == 1) return diag_extract_2d(self, diagonal);
        if (d1 == 1 && d2 == 0) return diag_extract_2d(self.transpose(0, 1), diagonal);
        return not_impl(op, args, self);
      }
    }

    // histc(bins, min, max)
    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const auto bins = (std::int64_t)as_i64(args[0]);
      const double mn = as_double(args[1]);
      const double mx = as_double(args[2]);
      if (op == "histc") return self.histc(bins, mn, mx);
      if (op == "histcounts") return self.histc(bins, mn, mx);
      if (op == "nan_to_num") return ops_internal::nan_to_num(self, as_double(args[0]), mn, mx);
    }

    // int,int,int for narrow/slice/select
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<std::int64_t>(args[1].v) &&
        std::holds_alternative<std::int64_t>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const auto start = as_i64(args[1]);
      const auto len = as_i64(args[2]);
      const auto wd = wrap_dim(dim, self.dim(), "narrow/slice/select");
      const auto dsz = self.sizes()[(std::size_t)wd];
      auto st = start;
      if (st < 0) st += dsz;

      if (op == "narrow" || op == "narrow_copy") {
        // Single-axis range select with an independent (freshly materialized)
        // result. A start or length that runs past the axis is an error --
        // the saturating variant is `slice`, which follows Python slice
        // semantics instead.
        PLUSS_CHECK(len >= 0,
                    std::string(op) + ": length must be non-negative, got " +
                        std::to_string(len));
        PLUSS_CHECK(st >= 0 && st <= dsz,
                    std::string(op) + ": start " + std::to_string(start) +
                        " is out of range for axis " + std::to_string(wd) +
                        " of size " + std::to_string(dsz));
        PLUSS_CHECK(st + len <= dsz,
                    std::string(op) + ": start + length (" +
                        std::to_string(st) + " + " + std::to_string(len) +
                        ") exceeds axis " + std::to_string(wd) + " size " +
                        std::to_string(dsz));
        // select_range() routes through gather(), which registers a proper
        // backward (scatter-add back into a zero-padded input shape) and
        // returns an independent buffer (the native gather kernels never
        // alias the source), so the storage-independence contract holds with
        // no extra copy. A trailing .clone() that once sat here was silently
        // detaching the result from the autograd graph -- Series::clone()
        // registers no backward, unlike Series::contiguous() -- so gradients
        // could never reach a parameter through a narrow_copy(); confirmed in
        // a real training loop. See PlussAI/Ice/ICE_BLUEPRINT.md's TFT
        // section for the original repro.
        return select_range(self, wd, st, len);
      }

      // slice / select / slice_inverse: saturating start, Python slice style.
      st = std::clamp<std::int64_t>(st, 0, dsz);
      if (op == "slice") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st));
      if (op == "select") {
        // Route through the core Series::select implementation so reverse-mode
        // follows the dedicated select backward path.
        return self.select(wd, st);
      }
      if (op == "slice_inverse") return select_range(self, wd, st, std::min<std::int64_t>(len, dsz - st));
    }

    if (std::holds_alternative<Arg::IntList>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto sizes = as_intlist(args[0]);
      const auto strides = as_intlist(args[1]);
      const auto off = as_i64(args[2]);
      if (op == "as_strided" || op == "_reshape_alias") return as_strided_impl(self, sizes, strides, off);
      return not_impl(op, args, self);
    }

    // addcmul(self, tensor1, tensor2, value)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      const Series& other = as_series(args[0]);
      const double rtol = as_double(args[1]);
      const double atol = as_double(args[2]);
      if (op == "select_scatter") return select_scatter_impl(self, other, as_i64(args[1]), as_i64(args[2]));
      if (op == "slice_scatter") return slice_scatter_impl(self, other, as_i64(args[1]), as_i64(args[2]), self.sizes()[(std::size_t)wrap_dim(as_i64(args[1]), self.dim(), "slice_scatter")], 1);
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, rtol, atol, false));
      if (op == "isclose") return isclose_impl(self, other, rtol, atol, false);
    }

    // addcmul(self, tensor1, tensor2, value)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        (std::holds_alternative<double>(args[2].v) || std::holds_alternative<std::int64_t>(args[2].v) || std::holds_alternative<Scalar>(args[2].v))) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double v = as_double(args[2]);
      if (op == "addcmul") return ops_internal::add(self, mul_scalar(ops_internal::mul(b, c), v));
      if (op == "addcdiv") return ops_internal::add(self, mul_scalar(ops_internal::div(b, c), v));
    }
    // put(index, src, accumulate)
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put") return put_dispatch_copy_(self, index, src, accumulate);
      if (op == "put") {
        return put_dispatch_copy_(self, index, src, accumulate);
      }
      if (op == "triangular_solve") return pluss::subsp::linearalg::triangular_solve(index, self, /*upper=*/true, /*unitriangular=*/accumulate);
    }

    if (std::holds_alternative<Arg::SeriesList>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto& indices = as_series_list(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put") return put_dispatch_copy_indices_(self, indices, src, accumulate);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        std::holds_alternative<Arg::IntList>(args[2].v)) {
      const auto& src = as_series(args[0]);
      const auto sizes = as_intlist(args[1]);
      const auto strides = as_intlist(args[2]);
      if (op == "as_strided_scatter") {
        return as_strided_scatter_impl(self, src, sizes, strides, /*storage_offset_elems=*/0);
      }
    }

    return not_impl(op, args, self);
  }

  // 4-arg
  if (args.size() == 4) {
    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      if (op == "slice") {
        return slice_view_impl(
            self,
            as_i64(args[0]),
            as_i64(args[1]),
            as_i64(args[2]),
            as_i64(args[3]));
      }
      if (op == "block") {
        return block_2d_impl(self, as_i64(args[0]), as_i64(args[1]), as_i64(args[2]), as_i64(args[3]));
      }
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        std::holds_alternative<Arg::IntList>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto& src = as_series(args[0]);
      const auto sizes = as_intlist(args[1]);
      const auto strides = as_intlist(args[2]);
      const auto off = as_i64(args[3]);
      if (op == "as_strided_scatter") return as_strided_scatter_impl(self, src, sizes, strides, off);
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& src = as_series(args[0]);
      if (op == "slice_scatter") {
        return slice_scatter_impl(self, src, as_i64(args[1]), as_i64(args[2]), as_i64(args[3]), /*step=*/1);
      }
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<bool>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& other = as_series(args[0]);
      const bool upper = as_bool(args[1]);
      const bool transpose = as_bool(args[2]);
      const bool unitriangular = as_bool(args[3]);
      if (op == "triangular_solve") {
        auto a = transpose ? other.transpose(0, 1) : other;
        return pluss::subsp::linearalg::triangular_solve(a, self, upper, unitriangular);
      }
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put" || op == "put") {
        return put_dispatch_copy_(self, index, src, accumulate);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const auto k = as_i64(args[0]);
      const auto dim = as_i64(args[1]);
      const bool largest = as_bool(args[2]);
      const bool sorted = as_bool(args[3]);
      if (op == "topk") return topk_values_only(self, k, dim, largest, sorted);
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const double alpha = as_double(args[3]);
      if (op == "index_add") {
        return index_add_impl_(self, dim, index, as_series(args[2]), alpha);
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      if (op == "index_reduce") {
        return index_reduce_impl_(self, dim, index, as_series(args[2]), reduce, /*include_self=*/true);
      }
      if (op == "scatter_reduce") {
        return ops_internal::scatter_reduce(self, dim, index, src, std::string(reduce), /*include_self=*/true);
      }
      return not_impl(op, args, self);
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& other = as_series(args[0]);
      const double rtol = as_double(args[1]);
      const double atol = as_double(args[2]);
      const bool equal_nan = as_bool(args[3]);
      if (op == "rrelu_with_noise") return rrelu_with_noise_impl_(self, other, rtol, atol, equal_nan);
      if (op == "allclose") return ops_internal::all(isclose_impl(self, other, rtol, atol, equal_nan));
      if (op == "isclose") return isclose_impl(self, other, rtol, atol, equal_nan);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double beta = as_double(args[2]);
      const double alpha = as_double(args[3]);
      if (op == "addmm") {
        check_mm_contract_(b, c, "addmm");
        return combine_beta_alpha(self, matmul_with_tape_(b, c), beta, alpha);
      }
      if (op == "addmv") {
        check_mv_contract_(b, c, "addmv");
        return combine_beta_alpha(self, matmul_with_tape_(b, c), beta, alpha);
      }
      if (op == "addr") {
        check_outer_contract_(b, c, "addr");
        return combine_beta_alpha(self, matmul_with_tape_(b.unsqueeze(1), c.unsqueeze(0)), beta, alpha);
      }
      if (op == "addbmm") {
        Series upd = matmul_with_tape_(b, c).sum(/*dim=*/0, /*keepdim=*/false);
        return combine_beta_alpha(self, upd, beta, alpha);
      }
      if (op == "baddbmm") {
        check_bmm_contract_(b, c, "baddbmm");
        return combine_beta_alpha(self, matmul_with_tape_(b, c), beta, alpha);
      }
    }
    return not_impl(op, args, self);
  }

  // 5-arg
  if (args.size() == 5) {
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3]) &&
        is_numeric_scalar_arg(args[4])) {
      const Series& src = as_series(args[0]);
      if (op == "slice_scatter") {
        return slice_scatter_impl(self, src, as_i64(args[1]), as_i64(args[2]), as_i64(args[3]), as_i64(args[4]));
      }
      return not_impl(op, args, self);
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      if (op == "index_reduce") {
        return index_reduce_impl_(self, dim, index, as_series(args[2]), reduce, include_self);
      }
      if (op == "scatter_reduce") {
        return ops_internal::scatter_reduce(self, dim, index, src, std::string(reduce), include_self);
      }
    }

    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3]) &&
        std::holds_alternative<bool>(args[4].v)) {
      if (op == "_addmm_activation") {
        const Series& mat1 = as_series(args[0]);
        const Series& mat2 = as_series(args[1]);
        const double beta = as_double(args[2]);
        const double alpha = as_double(args[3]);
        const bool use_gelu = as_bool(args[4]);
        check_mm_contract_(mat1, mat2, "_addmm_activation");
        const Series linear = combine_beta_alpha(self, matmul_with_tape_(mat1, mat2), beta, alpha);
        return use_gelu ? ops_internal::gelu(linear, /*approximate=*/false)
                        : ops_internal::relu(linear);
      }
    }
    return not_impl(op, args, self);
  }

  if (args.size() == 7) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v) &&
        std::holds_alternative<bool>(args[5].v) &&
        is_numeric_scalar_arg(args[6])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      const bool has_initial = as_bool(args[5]);
      const double initial = as_double(args[6]);
      if (op == "scatter_reduce") {
        return ops_internal::scatter_reduce(
            self,
            dim,
            index,
            src,
            std::string(reduce),
            include_self,
            has_initial,
            initial);
      }
    }
    return not_impl(op, args, self);
  }

    return not_impl(op, args, self);
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const std::exception& e) {
    rethrow_dispatch_error_("out-of-place", op_snapshot, e);
  } catch (...) {
    throw_error(
        std::string("CodeM out-of-place dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

Series& invoke_any_(std::string_view op, Series& self, const Args& args) {
  const std::string op_snapshot(op);
  try {
    op = canonical_inplace_op(op);
    ops_internal::PlatformScope platform_scope(self.platform());
    validate_series_arg_platforms(op, self, args);
    check_inplace_autograd_write_(op, self);
    const bool tracked_mutation_with_bridge =
        inplace_functional_bridge_enabled_() &&
        inplace_op_mutates_value_(op) &&
        series_tape::recording_enabled() &&
        series_tape::tracks_slope(self) &&
        inplace_op_has_functional_bridge_(op);
    const bool guard_version_bump =
        inplace_op_mutates_value_(op) &&
        !tracked_mutation_with_bridge &&
        op != "copy_" &&
        op != "put_" &&
        op != "index_put_";
    InplaceVersionGuard version_guard(self, guard_version_bump);

  if (tracked_mutation_with_bridge) {
    const Series bridge_input = make_inplace_bridge_input_snapshot_(self);
    InplaceBridgeArgs bridge_args = remap_inplace_bridge_args_to_snapshot_(args, self, bridge_input);
    Series value = invoke_any(functional_bridge_target_op_(op), bridge_input, bridge_args.args);
    if (value.defined() && self.defined()) {
      const std::vector<std::int64_t> self_shape(self.sizes().begin(), self.sizes().end());
      const bool same_rank = value.sizes().size() == self_shape.size();
      bool same_shape = same_rank;
      if (same_shape) {
        for (std::size_t i = 0; i < self_shape.size(); ++i) {
          if (value.sizes()[i] != self_shape[i]) {
            same_shape = false;
            break;
          }
        }
      }
      if (!same_shape) {
        if (self_shape.empty()) {
          value = value.reshape(self_shape);
        } else {
          value = value.expand(self_shape, /*allow_copy=*/true);
        }
      }
    }
    return copy_dispatch_inplace_(self, value);
  }

  if (args.empty()) {
    if (op == "_coalesced_" || op == "sparse_resize_" || op == "resize_as_sparse_") return self;
    if (op == "sparse_resize_and_clear_") {
      return inplace_result_(self, Series::zeros(self.sizes(), self.scalar_type()));
    }
    if (op == "digamma_") { check_special_inplace_dtype_(self, "digamma_"); return assign_inplace_preserve_storage_(self, digamma_impl(self).to(self.scalar_type())); }
    if (op == "erfinv_") { check_special_inplace_dtype_(self, "erfinv_"); return assign_inplace_preserve_storage_(self, erfinv_impl(self).to(self.scalar_type())); }
    if (op == "i0_") { check_special_inplace_dtype_(self, "i0_"); return assign_inplace_preserve_storage_(self, i0_impl(self).to(self.scalar_type())); }

    if (op == "relu_") return ops_internal::relu_(self);
    if (op == "relu6_") { return inplace_result_(self, ops_internal::clamp(self, 0.0, 6.0)); }
    if (op == "sigmoid_") return ops_internal::sigmoid_(self);
    if (op == "hardsigmoid_") { return inplace_result_(self, hardsigmoid_impl_(self).to(self.scalar_type())); }
    if (op == "hardswish_") { return inplace_result_(self, hardswish_impl_(self).to(self.scalar_type())); }
    if (op == "softplus_") return ops_internal::softplus_(self, 1.0, 20.0);
    if (op == "mish_") { return inplace_result_(self, mish_impl_(self).to(self.scalar_type())); }
    if (op == "silu_") return ops_internal::silu_(self);
    if (op == "selu_") { return inplace_result_(self, selu_impl_(self).to(self.scalar_type())); }
    if (op == "hardtanh_") { return inplace_result_(self, ops_internal::clamp(self, -1.0, 1.0)); }
    if (op == "celu_") { return inplace_result_(self, celu_impl_(self, 1.0).to(self.scalar_type())); }
    if (op == "elu_") { return inplace_result_(self, elu_impl_(self, 1.0, 1.0, 1.0).to(self.scalar_type())); }
    if (op == "rrelu_") { return inplace_result_(self, rrelu_impl_(self, 0.125, 0.3333333333333333, /*training=*/false).to(self.scalar_type())); }
    if (op == "gelu_") return ops_internal::gelu_(self);

    if (op == "exp_") return ops_internal::exp_(self);
    if (op == "log_") return ops_internal::log_(self);
    if (op == "sqrt_") return ops_internal::sqrt_(self);
    if (op == "rsqrt_") { return inplace_result_(self, ops_internal::rsqrt(self)); }
    if (op == "log2_") { return inplace_result_(self, ops_internal::log2(self)); }
    if (op == "log10_") { return inplace_result_(self, ops_internal::log10(self)); }
    if (op == "exp2_") { return inplace_result_(self, ops_internal::exp2(self)); }
    if (op == "square_") { return inplace_result_(self, ops_internal::square(self)); }

    if (op == "sin_") return self.sin_();
    if (op == "cos_") return self.cos_();
    if (op == "tanh_") return self.tanh_();
    if (op == "tan_") return self.tan_();
    if (op == "asin_") return self.asin_();
    if (op == "acos_") return self.acos_();
    if (op == "atan_") return self.atan_();
    if (op == "sinh_") return self.sinh_();
    if (op == "cosh_") return self.cosh_();

    if (op == "floor_") return self.floor_();
    if (op == "ceil_") return self.ceil_();
    if (op == "round_") return self.round_();
    if (op == "trunc_") return self.trunc_();
    if (op == "reciprocal_") return self.reciprocal_();
    if (op == "logical_not_") return self.logical_not_();
    if (op == "log1p_") return self.log1p_();
    if (op == "expm1_") return self.expm1_();
    if (op == "erf_") return self.erf_();
    if (op == "erfc_") return self.erfc_();
    if (op == "lgamma_") return self.lgamma_();
    if (op == "asinh_") return self.asinh_();
    if (op == "acosh_") return self.acosh_();
    if (op == "atanh_") return self.atanh_();
    if (op == "erfcinv_") { return inplace_result_(self, erfcinv_impl(self).to(self.scalar_type())); }

    if (op == "conj_physical_") { return inplace_result_(self, ops_internal::conj(self)); }
    if (op == "neg_") { return inplace_result_(self, ops_internal::neg(self)); }
    if (op == "sign_" || op == "sgn_") {
      Series pos = ops_internal::gt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      Series neg = ops_internal::lt(self, scalar_like(self, 0.0)).to(ScalarType::Float64);
      return inplace_result_(self, ops_internal::sub(pos, neg).to(self.scalar_type()));
    }
    if (op == "fix_") { return inplace_result_(self, ops_internal::trunc(self)); }
    if (op == "frac_") { return inplace_result_(self, ops_internal::sub(self, ops_internal::floor(self))); }
    if (op == "deg2rad_") { return inplace_result_(self, mul_scalar(self, kPi / 180.0)); }
    if (op == "rad2deg_") { return inplace_result_(self, mul_scalar(self, 180.0 / kPi)); }
    if (op == "positive_") return self;
    if (op == "isnan_") return self.isnan_();
    if (op == "isinf_") return self.isinf_();
    if (op == "isfinite_") return self.isfinite_();
    if (op == "detach_") {
      series_tape::set_tracks_slope(self, false);
      series_tape::set_keep_sensitivity(self, false);
      series_tape::clear_source_link(self);
      series_tape::clear_sensitivity(self);
      return self;
    }
    if (op == "clear_sensitivity") {
      series_tape::clear_sensitivity(self);
      return self;
    }
    if (op == "tracks_slope_") {
      series_tape::set_tracks_slope(self, true);
      return self;
    }
    if (op == "rename_" || op == "set_") return self;
    if (op == "abs_") { return inplace_result_(self, ops_internal::abs(self)); }
    if (op == "signbit_") {
      return inplace_result_(self, ops_internal::lt(self, scalar_like(self, 0.0)).to(self.scalar_type()));
    }
    if (op == "bitwise_not_") return ops_internal::bitwise_not_(self);
    if (op == "squeeze_") { return inplace_result_(self, ops_internal::squeeze(self)); }
    if (op == "t_") {
      if (self.dim() == 2) self = self.transpose(0, 1);
      return self;
    }
    if (op == "tril_") { return inplace_result_(self, tril_triu_impl(self, /*diagonal=*/0, /*upper=*/false)); }
    if (op == "triu_") { return inplace_result_(self, tril_triu_impl(self, /*diagonal=*/0, /*upper=*/true)); }
    if (op == "zero_") { return inplace_result_(self, Series::zeros(self.sizes(), self.scalar_type())); }
    if (op == "uniform_") return assign_stochastic_inplace_(self, uniform_like(self, 0.0, 1.0));
    if (op == "normal_") return assign_stochastic_inplace_(self, normal_like(self, 0.0, 1.0));
    if (op == "log_normal_") {
      PLUSS_CHECK(is_floating_dtype(self.scalar_type()), "log_normal_: receiver must have a floating-point dtype");
      return assign_stochastic_inplace_(self, ops_internal::exp(normal_like(self, 1.0, 2.0)));
    }
    if (op == "exponential_") return assign_stochastic_inplace_(self, exponential_like(self, 1.0));
    if (op == "geometric_") return assign_stochastic_inplace_(self, geometric_like(self, 0.5));
    if (op == "cauchy_") return assign_stochastic_inplace_(self, cauchy_like(self, 0.0, 1.0));
    if (op == "bernoulli_") return assign_stochastic_inplace_(self, bernoulli_like(self, 0.5));
    if (is_inplace_dropout_family_(op)) {
      return inplace_result_(self, dropout_family_impl_(op, self, /*p=*/0.5, /*train=*/true).to(self.scalar_type()));
    }
    if (op == "sinc_") { check_special_inplace_dtype_(self, "sinc_"); return assign_inplace_preserve_storage_(self, sinc_impl(self).to(self.scalar_type())); }
    if (op == "random_")
      return assign_stochastic_inplace_(self, random_like(self, 0.0, random_default_upper_(self.scalar_type())));

    return not_impl_inplace(op, args, self);
  }

    if (args.size() == 1) {
    const auto& a0 = args[0].v;
    if (is_numeric_scalar_arg(args[0])) {
      const double s = as_double(args[0]);
      if (op == "add_") { return assign_inplace_preserve_storage_(self, add_scalar(self, s)); }
      if (op == "sub_") { return assign_inplace_preserve_storage_(self, sub_scalar(self, s)); }
      if (op == "mul_") { return assign_inplace_preserve_storage_(self, mul_scalar(self, s)); }
      if (op == "div_") { return assign_inplace_preserve_storage_(self, div_scalar(self, s)); }
      if (op == "fill_") { return inplace_result_(self, Series::full(self.sizes(), s, self.scalar_type())); }
      if (op == "pow_") { return assign_inplace_preserve_storage_(self, pow_scalar(self, s).to(self.scalar_type())); }
      if (op == "float_power_") {
        PLUSS_CHECK(self.scalar_type() == ScalarType::Float64 || self.scalar_type() == ScalarType::Complex128,
                    "float_power_: receiver must be float64 or complex128");
        return assign_inplace_preserve_storage_(self, float_power_impl_(self, scalar_like(self, s)));
      }
      if (op == "floor_divide_") {
        const bool keep_int = !is_floating_dtype(self.scalar_type()) &&
                              !is_complex_dtype(self.scalar_type()) && s == std::trunc(s);
        const Series divisor = keep_int ? scalar_like(self, s)
                                        : Series::full({}, s, ScalarType::Float64).to(self.platform());
        return assign_inplace_preserve_storage_(self, floor_divide_impl_(self, divisor));
      }
      if (op == "fmod_") { return inplace_result_(self, ops_internal::fmod(self, scalar_like(self, s))); }
      if (op == "remainder_") { return inplace_result_(self, ops_internal::remainder(self, scalar_like(self, s))); }
      if (op == "atan2_") { return inplace_result_(self, ops_internal::atan2(self, scalar_like(self, s))); }
      if (op == "minimum_") { return inplace_result_(self, ops_internal::minimum(self, scalar_like(self, s))); }
      if (op == "maximum_") { return inplace_result_(self, ops_internal::maximum(self, scalar_like(self, s))); }
      if (op == "eq_") { return inplace_result_(self, ops_internal::eq(self, scalar_like(self, s))); }
      if (op == "ne_") { return inplace_result_(self, ops_internal::ne(self, scalar_like(self, s))); }
      if (op == "lt_") { return inplace_result_(self, ops_internal::lt(self, scalar_like(self, s))); }
      if (op == "le_") { return inplace_result_(self, ops_internal::le(self, scalar_like(self, s))); }
      if (op == "gt_") { return inplace_result_(self, ops_internal::gt(self, scalar_like(self, s))); }
      if (op == "ge_") { return inplace_result_(self, ops_internal::ge(self, scalar_like(self, s))); }
      if (op == "heaviside_") { return assign_inplace_preserve_storage_(self, heaviside_impl(self, scalar_like(self, s))); }
      if (op == "celu_") { return inplace_result_(self, celu_impl_(self, s).to(self.scalar_type())); }
      if (op == "elu_") { return inplace_result_(self, elu_impl_(self, s, 1.0, 1.0).to(self.scalar_type())); }
      if (is_inplace_dropout_family_(op)) {
        return inplace_result_(self, dropout_family_impl_(op, self, s, /*train=*/true).to(self.scalar_type()));
      }
      if (op == "gcd_") { return assign_inplace_preserve_storage_(self, gcd_impl(self, scalar_like(self, s))); }
      if (op == "lcm_") { return assign_inplace_preserve_storage_(self, lcm_impl(self, scalar_like(self, s))); }
      if (op == "logit_") { return inplace_result_(self, logit_impl(self, s).to(self.scalar_type())); }
      if (op == "nextafter_") { return inplace_result_(self, nextafter_impl(self, s).to(self.scalar_type())); }
      if (op == "ldexp_") {
        check_special_inplace_dtype_(self, "ldexp_");
        return assign_inplace_preserve_storage_(self, ldexp_impl(self, s).to(self.scalar_type()));
      }
      if (op == "igamma_") { check_special_inplace_dtype_(self, "igamma_"); return assign_inplace_preserve_storage_(self, igamma_impl(self, scalar_like(self, s), /*complement=*/false).to(self.scalar_type())); }
      if (op == "igammac_") { check_special_inplace_dtype_(self, "igammac_"); return assign_inplace_preserve_storage_(self, igamma_impl(self, scalar_like(self, s), /*complement=*/true).to(self.scalar_type())); }
      if (op == "mvlgamma_") { check_special_inplace_dtype_(self, "mvlgamma_"); return assign_inplace_preserve_storage_(self, mvlgamma_impl(self, (std::int64_t)s).to(self.scalar_type())); }
      if (op == "polygamma_") { check_special_inplace_dtype_(self, "polygamma_"); return assign_inplace_preserve_storage_(self, polygamma_impl(self, (std::int64_t)s).to(self.scalar_type())); }
      if (op == "renorm_") { return inplace_result_(self, renorm_impl(self, s, /*dim=*/0, /*maxnorm=*/1.0).to(self.scalar_type())); }
      if (op == "bitwise_left_shift_") { return inplace_result_(self, bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/true)); }
      if (op == "bitwise_right_shift_") { return inplace_result_(self, bitshift_scalar_impl(self, (std::int64_t)s, /*left=*/false)); }
      if (op == "copysign_") {
        const double sign = std::signbit(s) ? -1.0 : 1.0;
        return inplace_result_(self, mul_scalar(ops_internal::abs(self), sign));
      }
      if (op == "logaddexp_") {
        return inplace_result_(self, ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(scalar_like(self, s)))));
      }
      if (op == "logaddexp2_") {
        return inplace_result_(self, ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(scalar_like(self, s)))));
      }
      if (op == "hypot_") {
        return inplace_result_(self, ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(scalar_like(self, s)))));
      }
      if (op == "xlogy_") { return inplace_result_(self, ops_internal::mul(self, ops_internal::log(scalar_like(self, s)))); }
      if (op == "leaky_relu_") return ops_internal::leaky_relu_(self, s);
      if (op == "clamp_min_") return ops_internal::clamp_min_(self, s);
      if (op == "clamp_max_") return ops_internal::clamp_max_(self, s);
      if (op == "squeeze_") { return inplace_result_(self, ops_internal::squeeze(self, (std::int64_t)s)); }
      if (op == "unsqueeze_") { return inplace_result_(self, self.unsqueeze((std::int64_t)s)); }
      if (op == "tril_") { return inplace_result_(self, tril_triu_impl(self, (std::int64_t)s, /*upper=*/false)); }
      if (op == "triu_") { return inplace_result_(self, tril_triu_impl(self, (std::int64_t)s, /*upper=*/true)); }
      if (op == "fill_diagonal_") return fill_diagonal_impl(self, s, /*wrap=*/false);
      if (op == "cumsum_") return self.cumsum_((std::int64_t)s, /*keepdim=*/false);
      if (op == "cumprod_") return self.cumprod_((std::int64_t)s, /*keepdim=*/false);
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, s, 0.0, 0.0);
      if (op == "bernoulli_") return assign_stochastic_inplace_(self, bernoulli_like(self, s));
      if (op == "normal_") return assign_stochastic_inplace_(self, normal_like(self, s, 1.0));
      if (op == "log_normal_") {
        PLUSS_CHECK(is_floating_dtype(self.scalar_type()), "log_normal_: receiver must have a floating-point dtype");
        PLUSS_CHECK(std::isfinite(s), "log_normal_: expected a finite mean");
        return assign_stochastic_inplace_(self, ops_internal::exp(normal_like(self, s, 1.0)));
      }
      if (op == "exponential_") return assign_stochastic_inplace_(self, exponential_like(self, s));
      if (op == "geometric_") return assign_stochastic_inplace_(self, geometric_like(self, s));
      if (op == "cauchy_") return assign_stochastic_inplace_(self, cauchy_like(self, s, 1.0));
      if (op == "random_") return assign_stochastic_inplace_(self, random_like(self, 0.0, s));
      return not_impl_inplace(op, args, self);
    }

    if (std::holds_alternative<const Series*>(a0)) {
      const Series& other = as_series(args[0]);
      if (op == "ldexp_") {
        check_special_inplace_dtype_(self, "ldexp_");
        return assign_inplace_preserve_storage_(self, ldexp_impl(self, other).to(self.scalar_type()));
      }
      if (op == "igamma_") { check_special_inplace_dtype_(self, "igamma_"); return assign_inplace_preserve_storage_(self, igamma_impl(self, other, /*complement=*/false).to(self.scalar_type())); }
      if (op == "igammac_") { check_special_inplace_dtype_(self, "igammac_"); return assign_inplace_preserve_storage_(self, igamma_impl(self, other, /*complement=*/true).to(self.scalar_type())); }
      if (op == "resize_as_") { return inplace_result_(self, Series::empty(other.sizes(), self.scalar_type())); }
      if (op == "resize_as_sparse_") { return inplace_result_(self, Series::empty(other.sizes(), self.scalar_type())); }
      if (op == "add_") { return assign_inplace_preserve_storage_(self, ops_internal::add(self, other)); }
      if (op == "sub_") { return assign_inplace_preserve_storage_(self, ops_internal::sub(self, other)); }
      if (op == "mul_") { return assign_inplace_preserve_storage_(self, ops_internal::mul(self, other)); }
      if (op == "div_") { return assign_inplace_preserve_storage_(self, ops_internal::div(self, other)); }
      if (op == "pow_") { return assign_inplace_preserve_storage_(self, ops_internal::pow(self, other).to(self.scalar_type())); }
      if (op == "float_power_") {
        PLUSS_CHECK(self.scalar_type() == ScalarType::Float64 || self.scalar_type() == ScalarType::Complex128,
                    "float_power_: receiver must be float64 or complex128");
        return assign_inplace_preserve_storage_(self, float_power_impl_(self, other));
      }
      if (op == "floor_divide_") { return assign_inplace_preserve_storage_(self, floor_divide_impl_(self, other)); }
      if (op == "fmod_") { return inplace_result_(self, ops_internal::fmod(self, other)); }
      if (op == "remainder_") { return inplace_result_(self, ops_internal::remainder(self, other)); }
      if (op == "atan2_") { return inplace_result_(self, ops_internal::atan2(self, other)); }
      if (op == "minimum_") { return inplace_result_(self, ops_internal::minimum(self, other)); }
      if (op == "maximum_") { return inplace_result_(self, ops_internal::maximum(self, other)); }
      if (op == "eq_") { return inplace_result_(self, ops_internal::eq(self, other)); }
      if (op == "ne_") { return inplace_result_(self, ops_internal::ne(self, other)); }
      if (op == "lt_") { return inplace_result_(self, ops_internal::lt(self, other)); }
      if (op == "le_") { return inplace_result_(self, ops_internal::le(self, other)); }
      if (op == "gt_") { return inplace_result_(self, ops_internal::gt(self, other)); }
      if (op == "ge_") { return inplace_result_(self, ops_internal::ge(self, other)); }
      if (op == "logical_and_") { return inplace_result_(self, ops_internal::logical_and(self, other)); }
      if (op == "logical_or_") { return inplace_result_(self, ops_internal::logical_or(self, other)); }
      if (op == "logical_xor_") { return inplace_result_(self, ops_internal::logical_xor(self, other)); }
      if (op == "bitwise_and_") return ops_internal::bitwise_and_(self, other);
      if (op == "bitwise_or_") return ops_internal::bitwise_or_(self, other);
      if (op == "bitwise_xor_") return ops_internal::bitwise_xor_(self, other);
      if (op == "bitwise_left_shift_") { return inplace_result_(self, bitshift_series_impl(self, other, /*left=*/true)); }
      if (op == "bitwise_right_shift_") { return inplace_result_(self, bitshift_series_impl(self, other, /*left=*/false)); }
      if (op == "nextafter_") { return inplace_result_(self, nextafter_impl(self, other).to(self.scalar_type())); }
      if (op == "heaviside_") { return assign_inplace_preserve_storage_(self, heaviside_impl(self, other)); }
      if (op == "gcd_") { return assign_inplace_preserve_storage_(self, gcd_impl(self, other)); }
      if (op == "lcm_") { return assign_inplace_preserve_storage_(self, lcm_impl(self, other)); }
      if (op == "fmax_") { return assign_inplace_preserve_storage_(self, fmax_fmin_impl_(self, other, /*is_max=*/true)); }
      if (op == "fmin_") { return assign_inplace_preserve_storage_(self, fmax_fmin_impl_(self, other, /*is_max=*/false)); }
      if (op == "copysign_") {
        Series mag = ops_internal::abs(self);
        Series pos = ops_internal::ge(other, scalar_like(other, 0.0));
        return inplace_result_(self, ops_internal::where(pos, mag, ops_internal::neg(mag)));
      }
      if (op == "logaddexp_") { return inplace_result_(self, ops_internal::log(ops_internal::add(ops_internal::exp(self), ops_internal::exp(other)))); }
      if (op == "logaddexp2_") { return inplace_result_(self, ops_internal::log2(ops_internal::add(ops_internal::exp2(self), ops_internal::exp2(other)))); }
      if (op == "hypot_") { return inplace_result_(self, ops_internal::sqrt(ops_internal::add(ops_internal::square(self), ops_internal::square(other)))); }
      if (op == "xlogy_") { return inplace_result_(self, ops_internal::mul(self, ops_internal::log(other))); }
      if (op == "type_as_") { return inplace_result_(self, self.to(other.scalar_type())); }
      if (op == "copy_") {
        return copy_dispatch_inplace_(self, other);
      }
      if (op == "set_" || op == "set_data") {
        return inplace_result_(self, (other.scalar_type() == self.scalar_type()) ? other : other.to(self.scalar_type()));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<bool>(a0)) {
      if (op == "tracks_slope_") {
        series_tape::set_tracks_slope(self, std::get<bool>(a0));
        return self;
      }
      if (op == "rrelu_") {
        return inplace_result_(self, rrelu_impl_(self, 0.125, 0.3333333333333333, std::get<bool>(a0)).to(self.scalar_type()));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(a0)) {
      auto sizes = as_intlist(args[0]);
      if (op == "resize_") { return inplace_result_(self, Series::empty(sizes, self.scalar_type())); }
      if (op == "as_strided_") {
        return inplace_result_(self, as_strided_impl(self, sizes, contiguous_strides(sizes)));
      }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 2) {
    const auto& a0 = args[0].v;
    const auto& a1 = args[1].v;
    if (op == "_amp_foreach_non_finite_check_and_unscale_" &&
        std::holds_alternative<const Series*>(a0) &&
        std::holds_alternative<const Series*>(a1)) {
      Series& found_inf = as_mutable_series_ref(args[0]);
      const Series& inv_scale = as_series(args[1]);
      PLUSS_CHECK(found_inf.defined() && found_inf.numel() == 1,
                  "_amp_foreach_non_finite_check_and_unscale_: found_inf must be a defined scalar Series");
      PLUSS_CHECK(inv_scale.defined() && inv_scale.numel() == 1,
                  "_amp_foreach_non_finite_check_and_unscale_: inv_scale must be a defined scalar Series");
      if (series_has_nonfinite_(self)) {
        write_scalar_value_(found_inf, 1.0);
      }
      const double inv = inv_scale.to(ScalarType::Float64).item<double>();
      return assign_inplace_preserve_storage_(self, mul_scalar(self, inv));
    }
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1])) {
      const double x = as_double(args[0]);
      const double y = as_double(args[1]);
      if (op == "clamp_") return ops_internal::clamp_(self, x, y);
      if (op == "hardtanh_") { return inplace_result_(self, ops_internal::clamp(self, x, y)); }
      if (op == "threshold_") { return inplace_result_(self, threshold_impl_(self, x, y).to(self.scalar_type())); }
      if (op == "rrelu_") { return inplace_result_(self, rrelu_impl_(self, x, y, /*training=*/false).to(self.scalar_type())); }
      if (op == "softplus_") return ops_internal::softplus_(self, x, y);
      if (op == "elu_") { return inplace_result_(self, elu_impl_(self, x, y, 1.0).to(self.scalar_type())); }
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, x, y, y);
      if (op == "transpose_" || op == "movedim_" || op == "moveaxis_" || op == "swapdims_" || op == "swapaxes_") {
        return inplace_result_(self, self.transpose((std::int64_t)x, (std::int64_t)y));
      }
      if (op == "lerp_") {
        return inplace_result_(self, ops_internal::add(self, mul_scalar(ops_internal::sub(scalar_like(self, x), self), y)));
      }
      if (op == "uniform_") return assign_stochastic_inplace_(self, uniform_like(self, x, y));
      if (op == "normal_") return assign_stochastic_inplace_(self, normal_like(self, x, y));
      if (op == "log_normal_") {
        PLUSS_CHECK(is_floating_dtype(self.scalar_type()), "log_normal_: receiver must have a floating-point dtype");
        PLUSS_CHECK(std::isfinite(x), "log_normal_: expected a finite mean");
        PLUSS_CHECK(std::isfinite(y) && y > 0.0, "log_normal_: expected stddev > 0");
        return assign_stochastic_inplace_(self, ops_internal::exp(normal_like(self, x, y)));
      }
      if (op == "cauchy_") return assign_stochastic_inplace_(self, cauchy_like(self, x, y));
      if (op == "random_") return assign_stochastic_inplace_(self, random_like(self, x, y));
      if (is_inplace_dropout_family_(op)) {
        return inplace_result_(self, dropout_family_impl_(op, self, x, /*train=*/(y != 0.0)).to(self.scalar_type()));
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<bool>(a1)) {
      const auto dim = as_i64(args[0]);
      const double p = as_double(args[0]);
      const bool keepdim = as_bool(args[1]);
      if (is_inplace_dropout_family_(op)) {
        return inplace_result_(self, dropout_family_impl_(op, self, p, keepdim).to(self.scalar_type()));
      }
      if (op == "fill_diagonal_") return fill_diagonal_impl(self, as_double(args[0]), keepdim);
      if (op == "cumsum_") return self.cumsum_(dim, keepdim);
      if (op == "cumprod_") return self.cumprod_(dim, keepdim);
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && is_numeric_scalar_arg(args[1])) {
      const Series& other = as_series(args[0]);
      const double s = as_double(args[1]);
      if (op == "add_") {
        return inplace_result_(self, ops_internal::add(self, mul_scalar(other, s)));
      }
      if (op == "sub_") {
        return inplace_result_(self, ops_internal::sub(self, mul_scalar(other, s)));
      }
      if (op == "lerp_") {
        return inplace_result_(self, ops_internal::add(self, mul_scalar(ops_internal::sub(other, self), s)));
      }
      if (op == "masked_fill_") return ops_internal::masked_fill_(self, other, s);
      if (op == "index_fill_") {
        return assign_inplace_preserve_storage_(self, index_fill_impl_(self, 0, other, s));
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) && std::holds_alternative<const Series*>(a1)) {
      const auto index_value = as_i64(args[0]);
      const Series& src = as_series(args[1]);
      if (op == "index_put_" || op == "put_") {
        Series index = Series::full({1}, static_cast<double>(index_value), ScalarType::Int64)
                           .to(self.platform(), ScalarType::Int64);
        return put_dispatch_inplace_(self, index, src, /*accumulate=*/false);
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<bool>(a1)) {
      const Series& other = as_series(args[0]);
      if (op == "copy_") {
        // non_blocking is currently ignored.
        return copy_dispatch_inplace_(self, other);
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(a0) && std::holds_alternative<const Series*>(a1)) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      if (op == "addmm_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), 1.0, 1.0)); }
      if (op == "addmv_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), 1.0, 1.0)); }
      if (op == "addr_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b.unsqueeze(1), c.unsqueeze(0)), 1.0, 1.0)); }
      if (op == "addbmm_") {
        return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c).sum(/*dim=*/0, /*keepdim=*/false), 1.0, 1.0));
      }
      if (op == "baddbmm_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), 1.0, 1.0)); }
      if (op == "masked_scatter_") return ops_internal::masked_scatter_(self, b, c);
      if (op == "masked_fill_") return ops_internal::masked_fill_(self, b, c);
      if (op == "index_put_") { return put_dispatch_inplace_(self, b, c, /*accumulate=*/false); }
      if (op == "put_") { return put_dispatch_inplace_(self, b, c, /*accumulate=*/false); }
      if (op == "lerp_") { return inplace_result_(self, ops_internal::add(self, ops_internal::mul(ops_internal::sub(b, self), c))); }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(a0) && std::holds_alternative<Arg::IntList>(a1)) {
      auto sizes = as_intlist(args[0]);
      auto strides = as_intlist(args[1]);
      if (op == "as_strided_") { return inplace_result_(self, as_strided_impl(self, sizes, strides)); }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 3) {
    if (is_numeric_scalar_arg(args[0]) &&
        is_numeric_scalar_arg(args[1]) &&
        std::holds_alternative<bool>(args[2].v)) {
      if (op == "rrelu_") {
        return inplace_result_(self, rrelu_impl_(self, as_double(args[0]), as_double(args[1]), as_bool(args[2])).to(self.scalar_type()));
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) && is_numeric_scalar_arg(args[1]) && is_numeric_scalar_arg(args[2])) {
      if (op == "elu_") {
        return inplace_result_(self, elu_impl_(self, as_double(args[0]), as_double(args[1]), as_double(args[2])).to(self.scalar_type()));
      }
      if (op == "renorm_") { return inplace_result_(self, renorm_impl(self, as_double(args[0]), as_i64(args[1]), as_double(args[2])).to(self.scalar_type())); }
      if (op == "nan_to_num_") return ops_internal::nan_to_num_(self, as_double(args[0]), as_double(args[1]), as_double(args[2]));
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2])) {
      if (op == "embedding_renorm_") {
        const auto dim = (self.dim() > 1) ? 1 : 0;
        return inplace_result_(self, renorm_impl(self, as_double(args[2]), dim, as_double(args[1])).to(self.scalar_type()));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::IntList>(args[0].v) &&
        std::holds_alternative<Arg::IntList>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      if (op == "as_strided_") {
        return inplace_result_(self, as_strided_impl(self, as_intlist(args[0]), as_intlist(args[1]), as_i64(args[2])));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v)) {
      const auto dim = as_i64(args[0]);
      const Series& index = as_series(args[1]);
      const Series& src = as_series(args[2]);
      if (op == "scatter_") {
        return ops_internal::scatter_(self, dim, index, src);
      }
      if (op == "scatter_add_") return ops_internal::scatter_add_(self, dim, index, src);
      if (op == "index_copy_") {
        return assign_inplace_preserve_storage_(self, index_copy_impl_(self, dim, index, as_series(args[2])));
      }
      if (op == "index_add_") {
        return assign_inplace_preserve_storage_(self, index_add_impl_(self, dim, index, as_series(args[2]), 1.0));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<std::int64_t>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const auto dim = as_i64(args[0]);
      const Series& index = as_series(args[1]);
      const double value = as_double(args[2]);
      if (op == "index_fill_") {
        return assign_inplace_preserve_storage_(self, index_fill_impl_(self, dim, index, value));
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put_") { return put_dispatch_inplace_(self, index, src, accumulate); }
      if (op == "put_") { return put_dispatch_inplace_(self, index, src, accumulate); }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<Arg::SeriesList>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v)) {
      const auto& indices = as_series_list(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put_") {
        return put_dispatch_inplace_indices_(self, indices, src, accumulate);
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double v = as_double(args[2]);
      if (op == "addcmul_") { return inplace_result_(self, ops_internal::add(self, mul_scalar(ops_internal::mul(b, c), v))); }
      if (op == "addcdiv_") { return inplace_result_(self, ops_internal::add(self, mul_scalar(ops_internal::div(b, c), v))); }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 4) {
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<bool>(args[2].v) &&
        std::holds_alternative<bool>(args[3].v)) {
      const Series& index = as_series(args[0]);
      const Series& src = as_series(args[1]);
      const bool accumulate = as_bool(args[2]);
      if (op == "index_put_" || op == "put_") {
        return put_dispatch_inplace_(self, index, src, accumulate);
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        is_numeric_scalar_arg(args[1]) &&
        is_numeric_scalar_arg(args[2]) &&
        std::holds_alternative<bool>(args[3].v)) {
      if (op == "rrelu_with_noise_") {
        const Series& noise = as_series(args[0]);
        self = rrelu_with_noise_impl_(self, noise, as_double(args[1]), as_double(args[2]), as_bool(args[3]))
                   .to(self.scalar_type());
        return self;
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        is_numeric_scalar_arg(args[3])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const double alpha = as_double(args[3]);
      if (op == "index_add_") {
        return assign_inplace_preserve_storage_(self, index_add_impl_(self, dim, index, as_series(args[2]), alpha));
      }
      return not_impl_inplace(op, args, self);
    }
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      auto src = as_series(args[2]).to(self.scalar_type());
      const auto reduce = as_string(args[3]);
      if (op == "index_reduce_") {
        return assign_inplace_preserve_storage_(
            self, index_reduce_impl_(self, dim, index, as_series(args[2]), reduce, /*include_self=*/true));
      }
      if (op == "scatter_reduce_") {
        return ops_internal::scatter_reduce_(self, dim, index, as_series(args[2]), std::string(reduce),
                                             /*include_self=*/true);
      }
      return not_impl_inplace(op, args, self);
    }
    if (std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3])) {
      const Series& b = as_series(args[0]);
      const Series& c = as_series(args[1]);
      const double beta = as_double(args[2]);
      const double alpha = as_double(args[3]);
      if (op == "addmm_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), beta, alpha)); }
      if (op == "addmv_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), beta, alpha)); }
      if (op == "addr_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b.unsqueeze(1), c.unsqueeze(0)), beta, alpha)); }
      if (op == "addbmm_") {
        return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c).sum(/*dim=*/0, /*keepdim=*/false), beta, alpha));
      }
      if (op == "baddbmm_") { return inplace_result_(self, combine_beta_alpha(self, matmul_dispatch_(b, c), beta, alpha)); }
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 5) {
    if (op == "_amp_update_scale_" &&
        std::holds_alternative<const Series*>(args[0].v) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        is_numeric_scalar_arg(args[2]) &&
        is_numeric_scalar_arg(args[3]) &&
        is_numeric_scalar_arg(args[4])) {
      Series& growth_tracker = as_mutable_series_ref(args[0]);
      const Series& found_inf = as_series(args[1]);
      const double growth_factor = as_double(args[2]);
      const double backoff_factor = as_double(args[3]);
      const std::int64_t growth_interval = as_i64(args[4]);
      PLUSS_CHECK(self.defined() && self.numel() == 1, "_amp_update_scale_: self must be a defined scalar Series");
      PLUSS_CHECK(growth_tracker.defined() && growth_tracker.numel() == 1,
                  "_amp_update_scale_: growth_tracker must be a defined scalar Series");
      PLUSS_CHECK(found_inf.defined() && found_inf.numel() == 1,
                  "_amp_update_scale_: found_inf must be a defined scalar Series");
      PLUSS_CHECK(growth_interval > 0, "_amp_update_scale_: growth_interval must be > 0");
      const double scale_value = self.to(ScalarType::Float64).item<double>();
      if (series_any_true_(found_inf)) {
        write_scalar_value_(self, scale_value * backoff_factor);
        write_scalar_value_(growth_tracker, 0.0);
        return self;
      }
      const double successful = growth_tracker.to(ScalarType::Float64).item<double>() + 1.0;
      if (successful >= static_cast<double>(growth_interval)) {
        const double grown_value = scale_value * growth_factor;
        if (std::isfinite(grown_value)) {
          write_scalar_value_(self, grown_value);
        }
        write_scalar_value_(growth_tracker, 0.0);
        return self;
      }
      write_scalar_value_(growth_tracker, successful);
      return self;
    }

    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v)) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const auto& src = as_series(args[2]);
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      if (op == "scatter_reduce_") {
        return ops_internal::scatter_reduce_(self, dim, index, src, std::string(reduce), include_self);
      }
      if (op == "index_reduce_") {
        return assign_inplace_preserve_storage_(
            self, index_reduce_impl_(self, dim, index, src, reduce, include_self));
      }
      return not_impl_inplace(op, args, self);
    }
    return not_impl_inplace(op, args, self);
  }

  if (args.size() == 7) {
    if (is_numeric_scalar_arg(args[0]) &&
        std::holds_alternative<const Series*>(args[1].v) &&
        std::holds_alternative<const Series*>(args[2].v) &&
        std::holds_alternative<std::string>(args[3].v) &&
        std::holds_alternative<bool>(args[4].v) &&
        std::holds_alternative<bool>(args[5].v) &&
        is_numeric_scalar_arg(args[6])) {
      const auto dim = as_i64(args[0]);
      const auto& index = as_series(args[1]);
      const auto& src = as_series(args[2]);
      const auto reduce = as_string(args[3]);
      const bool include_self = as_bool(args[4]);
      const bool has_initial = as_bool(args[5]);
      const double initial = as_double(args[6]);
      if (op == "scatter_reduce_") {
        return ops_internal::scatter_reduce_(
            self,
            dim,
            index,
            src,
            std::string(reduce),
            include_self,
            has_initial,
            initial);
      }
    }
    return not_impl_inplace(op, args, self);
  }

  if (op == "_fused_sgd_" && args.size() >= 9 && args.size() <= 11 &&
      std::holds_alternative<const Series*>(args[0].v) &&
      std::holds_alternative<const Series*>(args[1].v)) {
    const Series& grad = as_series(args[0]);
    Series& momentum_buffer = as_mutable_series_ref(args[1]);
    const double weight_decay = as_double(args[2]);
    const double momentum = as_double(args[3]);
    const double lr = as_numeric_or_series_scalar_double(args[4]);
    const double dampening = as_double(args[5]);
    const bool nesterov = as_bool(args[6]);
    const bool maximize = as_bool(args[7]);
    const bool is_first_step = as_bool(args[8]);
    const Series* grad_scale = args.size() > 9 ? as_optional_series_ptr(args[9]) : nullptr;
    const Series* found_inf = args.size() > 10 ? as_optional_series_ptr(args[10]) : nullptr;
    if (should_skip_amp_step_(found_inf)) return self;

    Series g = maybe_unscale_grad_(grad.to(self.scalar_type()), grad_scale);
    if (maximize) g = ops_internal::neg(g);
    if (weight_decay != 0.0) g = ops_internal::add(g, mul_scalar(self, weight_decay));

    Series update = g;
    if (momentum != 0.0) {
      const Series next_buffer = is_first_step
          ? g
          : ops_internal::add(mul_scalar(momentum_buffer, momentum), mul_scalar(g, 1.0 - dampening));
      assign_inplace_preserve_storage_(momentum_buffer, next_buffer.to(momentum_buffer.scalar_type()));
      update = nesterov
          ? ops_internal::add(g, mul_scalar(momentum_buffer.to(self.scalar_type()), momentum))
          : momentum_buffer.to(self.scalar_type());
    }
    return assign_inplace_preserve_storage_(self, ops_internal::sub(self, mul_scalar(update, lr)));
  }

  if (op == "_fused_adagrad_" && args.size() >= 8 && args.size() <= 10 &&
      std::holds_alternative<const Series*>(args[0].v) &&
      std::holds_alternative<const Series*>(args[1].v) &&
      std::holds_alternative<const Series*>(args[2].v)) {
    const Series& grad = as_series(args[0]);
    Series& state_sum = as_mutable_series_ref(args[1]);
    const Series& state_step = as_series(args[2]);
    const double lr = as_numeric_or_series_scalar_double(args[3]);
    const double lr_decay = as_double(args[4]);
    const double weight_decay = as_double(args[5]);
    const double eps = as_double(args[6]);
    const bool maximize = as_bool(args[7]);
    const Series* grad_scale = args.size() > 8 ? as_optional_series_ptr(args[8]) : nullptr;
    const Series* found_inf = args.size() > 9 ? as_optional_series_ptr(args[9]) : nullptr;
    if (should_skip_amp_step_(found_inf)) return self;

    const double step = step_from_tensor_(state_step);
    Series g = maybe_unscale_grad_(grad.to(self.scalar_type()), grad_scale);
    if (maximize) g = ops_internal::neg(g);
    if (weight_decay != 0.0) g = ops_internal::add(g, mul_scalar(self, weight_decay));

    const double clr = lr / (1.0 + (step - 1.0) * lr_decay);
    assign_inplace_preserve_storage_(
        state_sum,
        ops_internal::add(state_sum, ops_internal::mul(g.to(state_sum.scalar_type()),
                                                       g.to(state_sum.scalar_type()))));
    const Series denom = add_scalar(ops_internal::sqrt(state_sum.to(self.scalar_type())), eps);
    return assign_inplace_preserve_storage_(
        self,
        ops_internal::sub(self, mul_scalar(ops_internal::div(g, denom), clr)));
  }

  if ((op == "_fused_adam_" || op == "_fused_adamw_") &&
      args.size() >= 12 && args.size() <= 14 &&
      std::holds_alternative<const Series*>(args[0].v) &&
      std::holds_alternative<const Series*>(args[1].v) &&
      std::holds_alternative<const Series*>(args[2].v) &&
      std::holds_alternative<const Series*>(args[3].v) &&
      std::holds_alternative<const Series*>(args[4].v)) {
    const Series& grad = as_series(args[0]);
    Series& exp_avg = as_mutable_series_ref(args[1]);
    Series& exp_avg_sq = as_mutable_series_ref(args[2]);
    Series& max_exp_avg_sq = as_mutable_series_ref(args[3]);
    const Series& state_step = as_series(args[4]);
    const double lr = as_numeric_or_series_scalar_double(args[5]);
    const double beta1 = as_double(args[6]);
    const double beta2 = as_double(args[7]);
    const double weight_decay = as_double(args[8]);
    const double eps = as_double(args[9]);
    const bool amsgrad = as_bool(args[10]);
    const bool maximize = as_bool(args[11]);
    const Series* grad_scale = args.size() > 12 ? as_optional_series_ptr(args[12]) : nullptr;
    const Series* found_inf = args.size() > 13 ? as_optional_series_ptr(args[13]) : nullptr;
    if (should_skip_amp_step_(found_inf)) return self;

    const double step = step_from_tensor_(state_step);
    PLUSS_CHECK(step > 0.0, std::string(op) + ": state_step must be > 0 (caller should increment step before fused update)");
    Series g = maybe_unscale_grad_(grad.to(self.scalar_type()), grad_scale);
    if (maximize) g = ops_internal::neg(g);

    if (weight_decay != 0.0) {
      if (op == "_fused_adamw_") {
        assign_inplace_preserve_storage_(self, mul_scalar(self, 1.0 - lr * weight_decay));
      } else {
        g = ops_internal::add(g, mul_scalar(self, weight_decay));
      }
    }

    const Series one = scalar_like(self, 1.0);
    const Series beta1_s = scalar_like(self, beta1);
    const Series beta2_s = scalar_like(self, beta2);
    const Series one_minus_beta1 = sub_scalar(one, beta1);
    const Series one_minus_beta2 = sub_scalar(one, beta2);

    const Series g_exp_dtype = g.to(exp_avg.scalar_type());
    const Series g_sq_exp_dtype = ops_internal::mul(g_exp_dtype, g_exp_dtype);
    assign_inplace_preserve_storage_(
        exp_avg,
        ops_internal::add(ops_internal::mul(exp_avg, beta1_s.to(exp_avg.scalar_type())),
                          ops_internal::mul(g_exp_dtype, one_minus_beta1.to(exp_avg.scalar_type()))));
    assign_inplace_preserve_storage_(
        exp_avg_sq,
        ops_internal::add(ops_internal::mul(exp_avg_sq, beta2_s.to(exp_avg_sq.scalar_type())),
                          ops_internal::mul(g_sq_exp_dtype, one_minus_beta2.to(exp_avg_sq.scalar_type()))));

    const double bias_correction1 = 1.0 - std::pow(beta1, step);
    const double bias_correction2_sqrt = std::sqrt(1.0 - std::pow(beta2, step));
    const Series denom_base = [&]() -> Series {
      if (amsgrad) {
        assign_inplace_preserve_storage_(max_exp_avg_sq, ops_internal::maximum(max_exp_avg_sq, exp_avg_sq));
        return max_exp_avg_sq.to(self.scalar_type());
      }
      return exp_avg_sq.to(self.scalar_type());
    }();

    const Series denom = add_scalar(div_scalar(ops_internal::sqrt(denom_base), bias_correction2_sqrt), eps);
    const Series step_term = div_scalar(exp_avg.to(self.scalar_type()), bias_correction1);
    return assign_inplace_preserve_storage_(
        self,
        ops_internal::sub(self, mul_scalar(ops_internal::div(step_term, denom), lr)));
  }

    return not_impl_inplace(op, args, self);
  } catch (const ::pluss::Error&) {
    throw;
  } catch (const std::exception& e) {
    rethrow_dispatch_error_("in-place", op_snapshot, e);
  } catch (...) {
    throw_error(
        std::string("CodeM in-place dispatch failed for '") + op_snapshot +
        "': unknown non-std exception");
  }
}

} // namespace pluss::codem::_compat
