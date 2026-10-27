#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg.h"
#include "CodeM/compat/Invoke.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/SeriesTape.h"
#include "CodeM/core/TypePromotion.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::math::diff {

namespace detail {

inline bool same_shape_(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  if (a.sizes().size() != b.sizes().size()) return false;
  for (std::size_t i = 0; i < a.sizes().size(); ++i) {
    if (a.sizes()[i] != b.sizes()[i]) return false;
  }
  return true;
}

inline void require_defined_(const Series& x, const char* op) {
  if (!x.defined()) {
    ::pluss::throw_error(std::string("subsp::math::diff::") + op + ": expected defined Series");
  }
}

inline void require_same_platform_(const Series& a, const Series& b, const char* op) {
  if (a.platform() != b.platform()) {
    ::pluss::throw_error(std::string("subsp::math::diff::") + op + ": expected matching platforms");
  }
}

inline void require_same_shape_(const Series& a, const Series& b, const char* op) {
  if (!same_shape_(a, b)) {
    ::pluss::throw_error(std::string("subsp::math::diff::") + op + ": expected matching shapes");
  }
}

inline void require_float_dtype_(const Series& x, const char* op) {
  if (!x.dtype().is_floating() && !x.dtype().is_complex()) {
    ::pluss::throw_error(std::string("subsp::math::diff::") + op +
                             ": expected floating or complex dtype");
  }
}

inline Series zeros_like_(const Series& like) {
  return pluss::series::full_like(like, 0.0, like.scalar_type());
}

inline Series ones_like_(const Series& like) {
  return pluss::series::full_like(like, 1.0, like.scalar_type());
}

inline void set_tracks_slope_(const Series& value, bool enabled) {
  pluss::codem::series_tape::set_tracks_slope(value, enabled);
  if (!enabled) {
    pluss::codem::series_tape::clear_sensitivity(value);
  }
}

inline void ensure_tracks_slope_(const Series& value) {
  if (!value.tracks_slope()) {
    set_tracks_slope_(value, true);
  }
}

inline bool has_reverse_path_(const Series& value) {
  return value.tracks_slope() ||
         pluss::codem::series_tape::source_info(value).has_value();
}

class SensitivityRetentionScope final {
public:
  explicit SensitivityRetentionScope(const std::vector<Series>& values) {
    retained_.reserve(values.size());
    for (const auto& value : values) {
      retained_.emplace_back(
          value, pluss::codem::series_tape::keeps_sensitivity(value));
      pluss::codem::series_tape::set_keep_sensitivity(value, true);
    }
  }

  SensitivityRetentionScope(const SensitivityRetentionScope&) = delete;
  SensitivityRetentionScope& operator=(const SensitivityRetentionScope&) =
      delete;

  ~SensitivityRetentionScope() {
    for (const auto& [series, previous] : retained_) {
      pluss::codem::series_tape::set_keep_sensitivity(series, previous);
    }
  }

private:
  std::vector<std::pair<Series, bool>> retained_;
};

inline void reverse_with_context_(const Series& output, const char* op) {
  try {
    output.reverse();
  } catch (const std::exception& e) {
    ::pluss::throw_error(
        std::string("subsp::math::diff::") + op +
        ": reverse failed: " + e.what() +
        " (hint: verify no in-place mutation on traced inputs after graph capture)");
  }
}

inline void reverse_with_context_(const Series& output, const Series& sensitivity_output, const char* op) {
  try {
    output.reverse(sensitivity_output);
  } catch (const std::exception& e) {
    ::pluss::throw_error(
        std::string("subsp::math::diff::") + op +
        ": reverse with seed failed: " + e.what() +
        " (hint: verify seed shape/platform and graph mutation invariants)");
  }
}

inline Series flat_basis_like_(const Series& like, std::int64_t index) {
  require_defined_(like, "flat_basis_like");
  PLUSS_CHECK(index >= 0 && index < like.numel(), "subsp::math::diff::flat_basis_like: index out of range");
  Series basis = pluss::series::arange(0, like.numel(), 1, ScalarType::Int64).to(like.platform());
  basis = basis.eq(index).to(like.scalar_type());
  return basis.reshape(like.sizes());
}

inline Series stack_rows_(const std::vector<Series>& rows,
                          std::int64_t ncols,
                          ScalarType dtype,
                          pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::series::empty({0, ncols}, dtype).to(platform);
  if (rows.size() == 1) return rows.front();
  return rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 0);
}

inline Series stack_cols_(const std::vector<Series>& cols,
                          std::int64_t nrows,
                          ScalarType dtype,
                          pluss::codem::Platform platform) {
  if (cols.empty()) return pluss::series::empty({nrows, 0}, dtype).to(platform);
  if (cols.size() == 1) return cols.front();
  return cols.front().unite(std::vector<Series>(cols.begin() + 1, cols.end()), 1);
}

inline double scalar_item_(const Series& x, const char* op) {
  require_defined_(x, op);
  if (x.numel() != 1) {
    ::pluss::throw_error(std::string("subsp::math::diff::") + op + ": expected scalar Series");
  }
  return x.item<double>();
}

inline bool close_enough_(double analytic, double numeric, double atol, double rtol) {
  if (!std::isfinite(analytic) || !std::isfinite(numeric)) return false;
  const double tol = atol + rtol * std::abs(numeric);
  return std::abs(analytic - numeric) <= tol;
}

// Track C batch 6: QR's forward-mode formula needs Q and R together (see
// qr_q/qr_r below, which each recompute this and take one half -- simple
// over clever, matches this batch's general approach). Ported from
// torch/csrc/autograd/FunctionsManual.cpp's linalg_qr_jvp, m>=n branch only
// (the common "tall or square" case); m<n (wide matrices) throws loud
// rather than silently using a formula that doesn't apply, matching
// upstream's own restriction (its m<n branch needs mode='reduced', which
// pluss's qr_q/qr_r always effectively use here anyway, but the underlying
// math genuinely differs by branch and porting only one honestly).
inline std::pair<Series, Series> qr_jvp_(const Series& Q, const Series& R, const Series& dA) {
  const auto rank = Q.dim();
  const auto m = Q.sizes()[static_cast<std::size_t>(rank - 2)];
  const auto n = R.sizes()[static_cast<std::size_t>(rank - 1)];
  if (m < n) {
    ::pluss::throw_error(
        "subsp::math::diff::qr: forward-mode not implemented for m < n (wide matrices)");
  }
  // dA @ R^{-1}, computed via the transpose trick since only left-solves
  // are available: (dA R^{-1}) = X <=> X R = dA <=> R^H X^H = dA^H.
  const Series dARinv = pluss::subsp::linearalg::triangular_solve(R.mH(), dA.mH(), /*upper=*/false, false).mH();
  const Series QhdARinv = Q.mH().matmul(dARinv);
  const Series sym = QhdARinv.add(QhdARinv.mH());
  const Series sym_half_diag = sym.diagonal(0, -2, -1).mul(0.5).diag_embed();
  const Series dR_scaled = sym.triu().sub(sym_half_diag);  // syminv(sym(...))
  const Series dQ = dARinv.sub(Q.matmul(dR_scaled));
  const Series dR = dR_scaled.matmul(R);
  return {dQ, dR};
}

} // namespace detail

struct HvpOptions {
  double epsilon{1e-6};
  enum class Mode {
    Auto,
    ExactOnly,
    FiniteDifferenceOnly,
  };
  Mode mode{Mode::Auto};
};

enum class DisconnectedSensitivity {
  Zero,
  Undefined,
};

struct DerivativePolicy {
  // Fail loudly when an input is disconnected from the objective/output.
  // Default keeps mathematical zero behavior for disconnected inputs.
  bool strict_dependency{false};
  // Emit a warning when disconnected paths are materialized via policy fallback.
  bool warn_on_disconnected{true};
  // Select representation when an input is disconnected and strict mode is off.
  DisconnectedSensitivity disconnected_sensitivity{DisconnectedSensitivity::Zero};
  // Keep captured source links after reverse. Disable for one-shot
  // derivative extraction to reduce tape retention.
  bool keep_graph{true};
  // Accumulate on existing input sensitivities instead of clearing first.
  bool accumulate_input_sensitivity{false};
  // When enabled, build a sensitivity graph during reverse so returned
  // derivatives can be differentiated again.
  bool track_higher_order{false};
};

struct ReversePolicy {
  // Keep captured source links after reverse. Disable for one-shot graph usage.
  bool keep_graph{true};
  // Accumulate onto existing sensitivities instead of resetting them first.
  bool accumulate_sensitivity{false};
  // Build propagation graph for higher-order reverse mode.
  bool track_higher_order{false};
};

enum class JacobianMode {
  Reverse,
  Forward,
};

struct JacobianOptions {
  JacobianMode mode{JacobianMode::Reverse};
  // Route Jacobian assembly through lane transforms.
  bool vectorized{false};
  DerivativePolicy policy{};
};

struct HessianOptions {
  HvpOptions hvp{};
  DerivativePolicy policy{};
  // Route Hessian assembly through lane transforms.
  bool vectorized{false};
};

struct DerivativeAuditOptions {
  // Central-difference step size.
  double epsilon{1e-6};
  // Comparison tolerances for analytic-vs-numeric directional checks.
  double atol{1e-6};
  double rtol{1e-4};
  // Use extra axis-aligned lanes in addition to the global lane.
  bool include_axis_lanes{true};
  // HVP mode used by curvature audits.
  HvpOptions hvp{};
  // Dependency/graph policy propagated to derivative transforms.
  DerivativePolicy policy{};
};

struct DerivativeAuditReport {
  bool passed{false};
  // "gradient" or "curvature".
  std::string phase{};
  // First failing lane index, or -1 when there is no lane-local mismatch.
  std::int64_t lane_index{-1};
  // Directional analytic and numerical values at first mismatch.
  double analytic{std::numeric_limits<double>::quiet_NaN()};
  double numeric{std::numeric_limits<double>::quiet_NaN()};
  // |analytic - numeric| and atol + rtol * |numeric| at first mismatch.
  double abs_error{std::numeric_limits<double>::infinity()};
  double tolerance{0.0};
};

enum class LossReduction {
  None,
  Sum,
  Mean,
};

class ReverseDebugScope final {
public:
  explicit ReverseDebugScope(bool check_non_finite = true)
      : scope_(check_non_finite) {}

private:
  pluss::codem::series_tape::ReverseDiagnosticsScope scope_;
};

inline void set_reverse_debug(bool enabled, bool check_non_finite = true) {
  pluss::codem::series_tape::set_reverse_diagnostics(enabled, check_non_finite);
}

inline bool reverse_debug_enabled() {
  return pluss::codem::series_tape::reverse_diagnostics().enabled;
}

class TraceRecordingScope final {
public:
  explicit TraceRecordingScope(bool enabled = true) {
    if (!enabled) {
      pause_.emplace();
    }
  }

private:
  std::optional<pluss::codem::series_tape::PauseRecording> pause_{};
};

inline bool trace_recording_enabled() noexcept {
  return pluss::codem::series_tape::recording_enabled();
}

struct SlopeSource {
  bool linked{false};
  std::string op{};
  std::size_t parents{0};
  std::string capture_file{};
  std::string capture_function{};
  std::uint_least32_t capture_line{0};
};

inline SlopeSource slope_source(const Series& value) {
  SlopeSource out{};
  const auto info = pluss::codem::series_tape::source_info(value);
  if (!info.has_value()) return out;
  out.linked = true;
  out.op = info->op_name;
  out.parents = info->parent_count;
  out.capture_file = info->capture_file;
  out.capture_function = info->capture_function;
  out.capture_line = info->capture_line;
  return out;
}

class Trace {
public:
  Trace() = default;
  explicit Trace(const Series& value) : value_(value) {}
  Trace(const Series& value, bool tracks_slope) : value_(value) {
    detail::set_tracks_slope_(value, tracks_slope);
  }

  bool defined() const noexcept { return value_.has_value() && value_->defined(); }

  bool tracks_slope() const {
    if (!defined()) return false;
    return value_->tracks_slope();
  }

  bool is_leaf() const {
    if (!defined()) return true;
    return value_->is_leaf();
  }

  const Series& value() const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::value: undefined trace");
    }
    return *value_;
  }

  bool has_sensitivity() const {
    if (!defined()) return false;
    return value_->has_sensitivity();
  }

  Series sensitivity() const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::sensitivity: undefined trace");
    }
    return value_->sensitivity();
  }

  void keep_sensitivity(bool keep = true) {
    if (defined()) value_->retain_grad(keep);
  }

  void clear() {
    if (defined()) value_->clear();
  }

  Trace separate() const {
    if (!defined()) return Trace();
    return Trace(value_->detach());
  }

  void reverse() const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::reverse: undefined trace");
    }
    detail::reverse_with_context_(*value_, "Trace::reverse");
  }

  void reverse(const Series& sensitivity_output) const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::reverse: undefined trace");
    }
    detail::reverse_with_context_(*value_, sensitivity_output, "Trace::reverse");
  }

  void reverse(const ReversePolicy& policy) const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::reverse: undefined trace");
    }
    try {
      value_->reverse(
          policy.keep_graph,
          policy.accumulate_sensitivity,
          policy.track_higher_order);
    } catch (const std::exception& e) {
      ::pluss::throw_error(
          std::string("subsp::math::diff::Trace::reverse") +
          ": reverse failed: " + e.what() +
          " (hint: verify no in-place mutation on traced inputs after graph capture)");
    }
  }

  void reverse(const Series& sensitivity_output, const ReversePolicy& policy) const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::Trace::reverse: undefined trace");
    }
    try {
      value_->reverse(
          sensitivity_output,
          policy.keep_graph,
          policy.accumulate_sensitivity,
          policy.track_higher_order);
    } catch (const std::exception& e) {
      ::pluss::throw_error(
          std::string("subsp::math::diff::Trace::reverse") +
          ": reverse with seed failed: " + e.what() +
          " (hint: verify seed shape/platform and graph mutation invariants)");
    }
  }

  // Compatibility bridge while callers migrate to reverse().
  void propagate() const { reverse(); }
  void propagate(const Series& sensitivity_output) const { reverse(sensitivity_output); }
  void propagate(const ReversePolicy& policy) const { reverse(policy); }
  void propagate(const Series& sensitivity_output, const ReversePolicy& policy) const {
    reverse(sensitivity_output, policy);
  }

private:
  std::optional<Series> value_{};
};

inline Trace trace(const Series& value, bool tracks_slope = true) {
  return Trace(value, tracks_slope);
}

inline Trace fixed(const Series& value) {
  return Trace(value, false);
}

class DualTrace {
public:
  DualTrace() = default;

  DualTrace(const Series& value, const Series& tangent)
      : value_(value) {
    detail::require_defined_(value, "DualTrace");
    detail::require_defined_(tangent, "DualTrace");
    detail::require_same_platform_(value, tangent, "DualTrace");
    if (value.scalar_type() != tangent.scalar_type()) {
      ::pluss::throw_error("subsp::math::diff::DualTrace: expected matching dtypes");
    }

    Series aligned_tangent = tangent;
    if (!detail::same_shape_(value, tangent)) {
      if (value.numel() != tangent.numel()) {
        ::pluss::throw_error("subsp::math::diff::DualTrace: expected matching shapes");
      }
      aligned_tangent = tangent.reshape(value.sizes());
    }
    tangent_ = std::move(aligned_tangent);
  }

  bool defined() const noexcept { return value_.has_value() && tangent_.has_value(); }

  const Series& value() const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::DualTrace::value: undefined dual trace");
    }
    return *value_;
  }

  const Series& tangent() const {
    if (!defined()) {
      ::pluss::throw_error("subsp::math::diff::DualTrace::tangent: undefined dual trace");
    }
    return *tangent_;
  }

  DualTrace separate() const {
    if (!defined()) return DualTrace();
    return DualTrace(value_->detach(), tangent_->detach());
  }

private:
  std::optional<Series> value_{};
  std::optional<Series> tangent_{};
};

inline DualTrace trace(const Series& value, const Series& tangent) {
  return DualTrace(value, tangent);
}

inline DualTrace fixed(const Series& value, const Series& tangent_like) {
  detail::require_defined_(value, "fixed");
  detail::require_defined_(tangent_like, "fixed");
  detail::require_same_platform_(value, tangent_like, "fixed");
  return DualTrace(value, detail::zeros_like_(tangent_like));
}

inline Trace constant_like(const Trace&, const Series& value) {
  return fixed(value);
}

inline DualTrace constant_like(const DualTrace& like, const Series& value) {
  detail::require_defined_(value, "constant_like");
  return fixed(value, like.tangent());
}

inline Trace make_scalar_like_(const Trace& like, double value) {
  return fixed(pluss::series::full_like(like.value(), value, like.value().scalar_type()));
}

inline DualTrace make_scalar_like_(const DualTrace& like, double value) {
  return DualTrace(
      pluss::series::full_like(like.value(), value, like.value().scalar_type()),
      detail::zeros_like_(like.value()));
}

inline Trace add(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "add");
  detail::require_defined_(b.value(), "add");
  detail::require_same_platform_(a.value(), b.value(), "add");
  return Trace(a.value().add(b.value()));
}

inline DualTrace add(const DualTrace& a, const DualTrace& b) {
  return DualTrace(a.value().add(b.value()), a.tangent().add(b.tangent()));
}

inline Trace sub(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "sub");
  detail::require_defined_(b.value(), "sub");
  detail::require_same_platform_(a.value(), b.value(), "sub");
  return Trace(a.value().sub(b.value()));
}

inline DualTrace sub(const DualTrace& a, const DualTrace& b) {
  return DualTrace(a.value().sub(b.value()), a.tangent().sub(b.tangent()));
}

inline Trace mul(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "mul");
  detail::require_defined_(b.value(), "mul");
  detail::require_same_platform_(a.value(), b.value(), "mul");
  return Trace(a.value().mul(b.value()));
}

inline DualTrace mul(const DualTrace& a, const DualTrace& b) {
  return DualTrace(
      a.value().mul(b.value()),
      a.tangent().mul(b.value()).add(a.value().mul(b.tangent())));
}

inline Trace div(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "div");
  detail::require_defined_(b.value(), "div");
  detail::require_same_platform_(a.value(), b.value(), "div");
  return Trace(a.value().div(b.value()));
}

inline DualTrace div(const DualTrace& a, const DualTrace& b) {
  const Series numerator = a.tangent().mul(b.value()).sub(a.value().mul(b.tangent()));
  const Series denominator = b.value().square();
  return DualTrace(a.value().div(b.value()), numerator.div(denominator));
}

inline Trace neg(const Trace& x) {
  detail::require_defined_(x.value(), "neg");
  return Trace(x.value().neg());
}

inline DualTrace neg(const DualTrace& x) {
  return DualTrace(x.value().neg(), x.tangent().neg());
}

inline Trace pow(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "pow");
  detail::require_defined_(b.value(), "pow");
  detail::require_same_platform_(a.value(), b.value(), "pow");
  return Trace(a.value().pow(b.value()));
}

inline DualTrace pow(const DualTrace& a, const DualTrace& b) {
  const Series primal = a.value().pow(b.value());
  const Series tangent = primal.mul(
      b.tangent().mul(a.value().log()).add(b.value().mul(a.tangent().div(a.value()))));
  return DualTrace(primal, tangent);
}

inline Trace pow(const Trace& a, double s) {
  detail::require_defined_(a.value(), "pow");
  return Trace(a.value().pow(s));
}

inline DualTrace pow(const DualTrace& a, double s) {
  const Series primal = a.value().pow(s);
  const Series tangent = a.tangent().mul(s).mul(a.value().pow(s - 1.0));
  return DualTrace(primal, tangent);
}

// Generated unary primitive derivatives live in the same registry that
// drives reverse-mode links in OpsInternal.cpp.
#include "CodeM/generated/SeriesAutodiffDiffUnaryGen.inc"

inline Trace abs(const Trace& x) {
  detail::require_defined_(x.value(), "abs");
  return Trace(x.value().abs());
}

inline DualTrace abs(const DualTrace& x) {
  const Series pos = x.value().gt(0.0).to(x.value().scalar_type());
  const Series neg = x.value().lt(0.0).to(x.value().scalar_type());
  const Series slope = pos.sub(neg);
  return DualTrace(x.value().abs(), slope.mul(x.tangent()));
}

inline Trace leaky_relu(const Trace& x, double negative_slope = 0.01) {
  detail::require_defined_(x.value(), "leaky_relu");
  return Trace(pluss::codem::ops_internal::leaky_relu(x.value(), negative_slope));
}

inline DualTrace leaky_relu(const DualTrace& x, double negative_slope = 0.01) {
  const Series one = detail::ones_like_(x.value());
  const Series mask = x.value().gt(0.0).to(x.value().scalar_type());
  const Series slope = mask.add(one.sub(mask).mul(negative_slope));
  return DualTrace(pluss::codem::ops_internal::leaky_relu(x.value(), negative_slope), slope.mul(x.tangent()));
}

inline Trace silu(const Trace& x) {
  detail::require_defined_(x.value(), "silu");
  return Trace(x.value().silu());
}

inline DualTrace silu(const DualTrace& x) {
  const Series s = x.value().sigmoid();
  const Series one = detail::ones_like_(s);
  const Series slope = s.mul(one.add(x.value().mul(one.sub(s))));
  return DualTrace(x.value().silu(), slope.mul(x.tangent()));
}

inline Trace gelu(const Trace& x, bool approximate = false) {
  detail::require_defined_(x.value(), "gelu");
  return Trace(pluss::codem::ops_internal::gelu(x.value(), approximate));
}

inline DualTrace gelu(const DualTrace& x, bool approximate = false) {
  const Series primal = pluss::codem::ops_internal::gelu(x.value(), approximate);
  const Series slope = [&]() {
    if (approximate) {
      constexpr double kC = 0.79788456080286535588; // sqrt(2/pi)
      constexpr double kK = 0.044715;
      const Series x2 = x.value().square();
      const Series u = x.value().add(x2.mul(x.value()).mul(kK)).mul(kC);
      const Series t = u.tanh();
      const Series one = detail::ones_like_(t);
      const Series du = one.add(x2.mul(3.0 * kK)).mul(kC);
      return one.add(t).mul(0.5).add(x.value().mul(0.5).mul(one.sub(t.square())).mul(du));
    }
    constexpr double kInvSqrt2 = 0.70710678118654752440;
    constexpr double kInvSqrt2Pi = 0.39894228040143267794;
    const Series one = detail::ones_like_(x.value());
    const Series erf_term = x.value().mul(kInvSqrt2).erf();
    const Series exp_term = x.value().square().mul(-0.5).exp();
    return one.add(erf_term).mul(0.5).add(x.value().mul(kInvSqrt2Pi).mul(exp_term));
  }();
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace softmax(const Trace& x, std::int64_t axis = -1) {
  detail::require_defined_(x.value(), "softmax");
  return Trace(x.value().softmax(axis));
}

inline DualTrace softmax(const DualTrace& x, std::int64_t axis = -1) {
  const Series primal = x.value().softmax(axis);
  const Series dot = primal.mul(x.tangent()).sum(axis, true);
  const Series tangent = primal.mul(x.tangent().sub(dot));
  return DualTrace(primal, tangent);
}

inline Trace log_softmax(const Trace& x, std::int64_t axis = -1) {
  detail::require_defined_(x.value(), "log_softmax");
  return Trace(x.value().log_softmax(axis));
}

inline DualTrace log_softmax(const DualTrace& x, std::int64_t axis = -1) {
  const Series primal = x.value().log_softmax(axis);
  const Series probs = primal.exp();
  const Series tangent = x.tangent().sub(probs.mul(x.tangent().sum(axis, true)));
  return DualTrace(primal, tangent);
}

inline Trace softplus(const Trace& x, double beta = 1.0, double threshold = 20.0) {
  detail::require_defined_(x.value(), "softplus");
  return Trace(x.value().softplus(beta, threshold));
}

inline DualTrace softplus(const DualTrace& x, double beta = 1.0, double threshold = 20.0) {
  const Series primal = x.value().softplus(beta, threshold);
  const Series bx = x.value().mul(beta);
  const Series mask = bx.gt(threshold).to(x.value().scalar_type());
  const Series one = detail::ones_like_(primal);
  const Series slope = mask.add(one.sub(mask).mul(bx.sigmoid()));
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace clamp(const Trace& x, double min, double max) {
  detail::require_defined_(x.value(), "clamp");
  return Trace(x.value().clamp(min, max));
}

inline DualTrace clamp(const DualTrace& x, double min, double max) {
  const Series mask = x.value().gt(min).logical_and(x.value().lt(max)).to(x.value().scalar_type());
  return DualTrace(x.value().clamp(min, max), mask.mul(x.tangent()));
}

// ---------------------------------------------------------------------------
// Track C batch 5: the Track B fused activation family (elu/celu/selu/
// hardsigmoid/hardswish/mish/threshold), now given forward-mode duals too.
// These are all pointwise scalar functions, so unlike the structural/
// reduction batches above the JVP formula is literally the same
// "multiply by the local derivative" shape as the VJP -- these are the
// exact same analytic slope formulas from each op's fused reverse node in
// OpsInternal.cpp, just applied to the tangent instead of the cotangent.
// rrelu/dropout are deliberately NOT included: their randomness is drawn
// once per forward call, and forward-mode AD's use case (propagating a
// fixed direction through a numeric computation) doesn't have an
// established convention for what "the same random draw" even means across
// a swept batch of directions the way jvp_lanes/jacfwd would use them.
// ---------------------------------------------------------------------------

inline Trace elu(const Trace& x, double alpha = 1.0, double scale = 1.0, double input_scale = 1.0) {
  detail::require_defined_(x.value(), "elu");
  return Trace(x.value().elu(alpha, scale, input_scale));
}

inline DualTrace elu(const DualTrace& x, double alpha = 1.0, double scale = 1.0, double input_scale = 1.0) {
  const Series primal = x.value().elu(alpha, scale, input_scale);
  const Series one = detail::ones_like_(x.value());
  const Series pos_mask = x.value().gt(0.0).to(x.value().scalar_type());
  const Series neg_slope = x.value().mul(input_scale).exp().mul(alpha * scale * input_scale);
  const Series slope = pos_mask.mul(scale).add(one.sub(pos_mask).mul(neg_slope));
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace celu(const Trace& x, double alpha = 1.0) {
  detail::require_defined_(x.value(), "celu");
  return Trace(x.value().celu(alpha));
}

inline DualTrace celu(const DualTrace& x, double alpha = 1.0) {
  return elu(x, alpha, 1.0, 1.0 / alpha);
}

inline Trace selu(const Trace& x) {
  detail::require_defined_(x.value(), "selu");
  return Trace(x.value().selu());
}

inline DualTrace selu(const DualTrace& x) {
  constexpr double kSeluAlpha = 1.6732632423543772;
  constexpr double kSeluScale = 1.0507009873554805;
  return elu(x, kSeluAlpha, kSeluScale, 1.0);
}

inline Trace hardsigmoid(const Trace& x) {
  detail::require_defined_(x.value(), "hardsigmoid");
  return Trace(x.value().hardsigmoid());
}

inline DualTrace hardsigmoid(const DualTrace& x) {
  const Series primal = x.value().hardsigmoid();
  const Series mask = x.value().gt(-3.0).logical_and(x.value().lt(3.0)).to(x.value().scalar_type());
  return DualTrace(primal, mask.mul(1.0 / 6.0).mul(x.tangent()));
}

inline Trace hardswish(const Trace& x) {
  detail::require_defined_(x.value(), "hardswish");
  return Trace(x.value().hardswish());
}

inline DualTrace hardswish(const DualTrace& x) {
  const Series primal = x.value().hardswish();
  const Series one = detail::ones_like_(x.value());
  const Series lo_mask = x.value().le(-3.0).to(x.value().scalar_type());
  const Series hi_mask = x.value().ge(3.0).to(x.value().scalar_type());
  const Series mid_mask = one.sub(lo_mask).sub(hi_mask);
  const Series mid_slope = x.value().mul(2.0).add(3.0).mul(1.0 / 6.0);
  const Series slope = hi_mask.add(mid_mask.mul(mid_slope));
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace mish(const Trace& x) {
  detail::require_defined_(x.value(), "mish");
  return Trace(x.value().mish());
}

inline DualTrace mish(const DualTrace& x) {
  const Series primal = x.value().mish();
  const Series t = x.value().softplus(1.0, 20.0).tanh();
  const Series s = x.value().sigmoid();
  const Series one = detail::ones_like_(x.value());
  const Series slope = t.add(x.value().mul(s).mul(one.sub(t.mul(t))));
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace threshold(const Trace& x, double threshold_val, double value) {
  detail::require_defined_(x.value(), "threshold");
  return Trace(x.value().threshold(threshold_val, value));
}

inline DualTrace threshold(const DualTrace& x, double threshold_val, double value) {
  const Series primal = x.value().threshold(threshold_val, value);
  const Series mask = x.value().gt(threshold_val).to(x.value().scalar_type());
  return DualTrace(primal, mask.mul(x.tangent()));
}

// rrelu/dropout: DualTrace computes primal and tangent together in one
// call, so unlike a separate forward+backward pass there is no need to
// capture or recreate "the same random draw" -- the realized per-element
// slope/scale is recoverable arithmetically straight from the primal that
// was just computed (primal_i == x_i * realized_scale_i wherever the op is
// linear-conditional-on-a-drawn-constant, so realized_scale_i ==
// primal_i/x_i), without touching the compat-layer RNG at all.
inline Trace rrelu(const Trace& x, double lower, double upper, bool training = false) {
  detail::require_defined_(x.value(), "rrelu");
  return Trace(x.value().rrelu(lower, upper, training));
}

inline DualTrace rrelu(const DualTrace& x, double lower, double upper, bool training = false) {
  const Series primal = x.value().rrelu(lower, upper, training);
  const Series one = detail::ones_like_(x.value());
  const Series pos_mask = x.value().gt(0.0).to(x.value().scalar_type());
  if (!training) {
    const double mid = 0.5 * (lower + upper);
    const Series slope = pos_mask.add(one.sub(pos_mask).mul(mid));
    return DualTrace(primal, slope.mul(x.tangent()));
  }
  // x_i==0 forces primal_i==0 too (either branch gives 0 there), so
  // dividing by a substituted 1 at that single point safely yields a slope
  // of 0 -- a defensible value since the true drawn slope isn't recoverable
  // at that measure-zero point either way.
  const Series safe_x = x.value().where(x.value().eq(0.0), one);
  const Series recovered_slope = primal.div(safe_x);
  const Series slope = pos_mask.add(one.sub(pos_mask).mul(recovered_slope));
  return DualTrace(primal, slope.mul(x.tangent()));
}

// Plain/feature dropout: primal_i = x_i * scale_i, scale_i in {0, 1/keep}.
// Same recovery trick as rrelu above -- and it also happens to make
// `feature_dropout` (whose noise broadcasts across extra dims before
// multiplying) free: by the time we look at the elementwise primal/x
// ratio, the broadcasting is already baked in, so the exact same formula
// covers both without needing to know which one was used.
inline Trace dropout(const Trace& x, double p, bool train) {
  detail::require_defined_(x.value(), "dropout");
  return Trace(pluss::codem::_compat::invoke("dropout", x.value(), {p, train}));
}

inline DualTrace dropout(const DualTrace& x, double p, bool train) {
  const Series primal = pluss::codem::_compat::invoke("dropout", x.value(), {p, train});
  if (!train || p <= 0.0) {
    return DualTrace(primal, x.tangent());
  }
  if (p >= 1.0) {
    return DualTrace(primal, detail::zeros_like_(x.tangent()));
  }
  const Series one = detail::ones_like_(x.value());
  const Series zero = detail::zeros_like_(x.value());
  const Series self_is_zero = x.value().eq(0.0);
  const Series safe_self = x.value().where(self_is_zero, one);
  const Series recovered_scale = primal.div(safe_self).where(self_is_zero, zero);
  return DualTrace(primal, recovered_scale.mul(x.tangent()));
}

inline Trace feature_dropout(const Trace& x, double p, bool train) {
  detail::require_defined_(x.value(), "feature_dropout");
  return Trace(pluss::codem::_compat::invoke("feature_dropout", x.value(), {p, train}));
}

inline DualTrace feature_dropout(const DualTrace& x, double p, bool train) {
  const Series primal = pluss::codem::_compat::invoke("feature_dropout", x.value(), {p, train});
  if (!train || p <= 0.0) {
    return DualTrace(primal, x.tangent());
  }
  if (p >= 1.0) {
    return DualTrace(primal, detail::zeros_like_(x.tangent()));
  }
  const Series one = detail::ones_like_(x.value());
  const Series zero = detail::zeros_like_(x.value());
  const Series self_is_zero = x.value().eq(0.0);
  const Series safe_self = x.value().where(self_is_zero, one);
  const Series recovered_scale = primal.div(safe_self).where(self_is_zero, zero);
  return DualTrace(primal, recovered_scale.mul(x.tangent()));
}

// alpha_dropout deliberately NOT included: out_i = x_i*a*noise_i +
// alpha_a*(noise_i-1) + alpha_a*p has TWO free constants per element
// (scale AND shift) tied to the same single Bernoulli draw `noise_i`, so
// recovering it from the primal needs solving primal_i = noise_i*(x_i*a +
// alpha_a) - alpha_a*(1-p) for noise_i -- exact in principle, but the
// denominator x_i*a+alpha_a vanishes at x_i == -alpha_a/a == -alpha
// (~ -1.758 for the default alpha), a perfectly ordinary activation value,
// not a measure-zero edge case like rrelu/plain-dropout's x_i==0 -- so this
// reconstruction is numerically fragile in a way the other two aren't.

inline Trace sum(const Trace& x) {
  detail::require_defined_(x.value(), "sum");
  return Trace(x.value().sum());
}

inline DualTrace sum(const DualTrace& x) {
  return DualTrace(x.value().sum(), x.tangent().sum());
}

inline Trace mean(const Trace& x) {
  detail::require_defined_(x.value(), "mean");
  return Trace(x.value().mean());
}

inline DualTrace mean(const DualTrace& x) {
  return DualTrace(x.value().mean(), x.tangent().mean());
}

// ---------------------------------------------------------------------------
// Track C: per-dim reductions and structural (shape-only) ops. All of these
// are LINEAR in x, so their JVP is exactly "apply the same op to the
// tangent" -- e.g. d(sum(x,dim))/dx . v = sum(v,dim) for any direction v.
// That makes these rules exact by construction, not an approximation: no
// case analysis, no risk of a missed branch, unlike e.g. amax/amin (a
// selection, not linear) which are deliberately NOT included in this batch.
// ---------------------------------------------------------------------------

inline Trace sum(const Trace& x, std::int64_t dim, bool keepdim = false) {
  detail::require_defined_(x.value(), "sum");
  return Trace(x.value().sum(dim, keepdim));
}

inline DualTrace sum(const DualTrace& x, std::int64_t dim, bool keepdim = false) {
  return DualTrace(x.value().sum(dim, keepdim), x.tangent().sum(dim, keepdim));
}

inline Trace sum(const Trace& x, pluss::msharp::RangeView::Int dims, bool keepdim = false) {
  detail::require_defined_(x.value(), "sum");
  return Trace(x.value().sum(dims, keepdim));
}

inline DualTrace sum(const DualTrace& x, pluss::msharp::RangeView::Int dims, bool keepdim = false) {
  return DualTrace(x.value().sum(dims, keepdim), x.tangent().sum(dims, keepdim));
}

inline Trace mean(const Trace& x, std::int64_t dim, bool keepdim = false) {
  detail::require_defined_(x.value(), "mean");
  return Trace(x.value().mean(dim, keepdim));
}

inline DualTrace mean(const DualTrace& x, std::int64_t dim, bool keepdim = false) {
  return DualTrace(x.value().mean(dim, keepdim), x.tangent().mean(dim, keepdim));
}

inline Trace mean(const Trace& x, pluss::msharp::RangeView::Int dims, bool keepdim = false) {
  detail::require_defined_(x.value(), "mean");
  return Trace(x.value().mean(dims, keepdim));
}

inline DualTrace mean(const DualTrace& x, pluss::msharp::RangeView::Int dims, bool keepdim = false) {
  return DualTrace(x.value().mean(dims, keepdim), x.tangent().mean(dims, keepdim));
}

inline Trace reshape(const Trace& x, pluss::msharp::RangeView::Int new_sizes) {
  detail::require_defined_(x.value(), "reshape");
  return Trace(x.value().reshape(new_sizes));
}

inline DualTrace reshape(const DualTrace& x, pluss::msharp::RangeView::Int new_sizes) {
  return DualTrace(x.value().reshape(new_sizes), x.tangent().reshape(new_sizes));
}

inline Trace transpose(const Trace& x, std::int64_t dim0, std::int64_t dim1) {
  detail::require_defined_(x.value(), "transpose");
  return Trace(x.value().transpose(dim0, dim1));
}

inline DualTrace transpose(const DualTrace& x, std::int64_t dim0, std::int64_t dim1) {
  return DualTrace(x.value().transpose(dim0, dim1), x.tangent().transpose(dim0, dim1));
}

inline Trace squeeze(const Trace& x) {
  detail::require_defined_(x.value(), "squeeze");
  return Trace(x.value().squeeze());
}

inline DualTrace squeeze(const DualTrace& x) {
  return DualTrace(x.value().squeeze(), x.tangent().squeeze());
}

inline Trace squeeze(const Trace& x, std::int64_t dim) {
  detail::require_defined_(x.value(), "squeeze");
  return Trace(x.value().squeeze(dim));
}

inline DualTrace squeeze(const DualTrace& x, std::int64_t dim) {
  return DualTrace(x.value().squeeze(dim), x.tangent().squeeze(dim));
}

inline Trace unsqueeze(const Trace& x, std::int64_t dim) {
  detail::require_defined_(x.value(), "unsqueeze");
  return Trace(x.value().unsqueeze(dim));
}

inline DualTrace unsqueeze(const DualTrace& x, std::int64_t dim) {
  return DualTrace(x.value().unsqueeze(dim), x.tangent().unsqueeze(dim));
}

inline Trace flatten(const Trace& x, std::int64_t start_dim, std::int64_t end_dim) {
  detail::require_defined_(x.value(), "flatten");
  return Trace(x.value().flatten(start_dim, end_dim));
}

inline DualTrace flatten(const DualTrace& x, std::int64_t start_dim, std::int64_t end_dim) {
  return DualTrace(x.value().flatten(start_dim, end_dim), x.tangent().flatten(start_dim, end_dim));
}

// ---------------------------------------------------------------------------
// Track C batch 2: amax/amin (a selection, NOT linear -- unlike everything
// above) and var/std (a smooth quadratic, not linear either). Both need a
// real derivative, not the "apply the same op to the tangent" shortcut.
// ---------------------------------------------------------------------------

// amax's JVP at x: the max is attained at every index where x_i equals the
// max (ties split evenly, matching the reverse-mode rule in
// maybe_link_extrema_reduce_ in OpsInternal.cpp -- both directions need the
// same subgradient convention at a tie to be consistent with each other).
// d(amax)/dx . v = sum_dim( (1{x_i==max}/count_ties) * v_i ).
inline Trace amax(const Trace& x, std::int64_t dim, bool keepdim = false) {
  detail::require_defined_(x.value(), "amax");
  return Trace(x.value().amax(dim, keepdim));
}

inline DualTrace amax(const DualTrace& x, std::int64_t dim, bool keepdim = false) {
  const Series primal = x.value().amax(dim, keepdim);
  const Series value_bc = (keepdim ? primal : primal.unsqueeze(dim)).expand(x.value().sizes(), true);
  const Series mask = x.value().eq(value_bc).to(x.value().scalar_type());
  const Series count = mask.sum(dim, true);
  const Series weight = mask.div(count);
  return DualTrace(primal, weight.mul(x.tangent()).sum(dim, keepdim));
}

inline Trace amin(const Trace& x, std::int64_t dim, bool keepdim = false) {
  detail::require_defined_(x.value(), "amin");
  return Trace(x.value().amin(dim, keepdim));
}

inline DualTrace amin(const DualTrace& x, std::int64_t dim, bool keepdim = false) {
  const Series primal = x.value().amin(dim, keepdim);
  const Series value_bc = (keepdim ? primal : primal.unsqueeze(dim)).expand(x.value().sizes(), true);
  const Series mask = x.value().eq(value_bc).to(x.value().scalar_type());
  const Series count = mask.sum(dim, true);
  const Series weight = mask.div(count);
  return DualTrace(primal, weight.mul(x.tangent()).sum(dim, keepdim));
}

// var(x,dim) = mean((x-mean(x,dim))^2, dim) [* n/(n-1) if unbiased]. Letting
// c = x - mean(x,dim) (the centered value), d(var)/dx . v =
// 2*mean(c*dc, dim) where dc = v - mean(v,dim) is the tangent of c itself
// (mean is linear, so its own tangent is just mean(v,dim)).
inline Trace var(const Trace& x, std::int64_t dim, bool unbiased, bool keepdim = false) {
  detail::require_defined_(x.value(), "var");
  return Trace(x.value().var(dim, unbiased, keepdim));
}

inline DualTrace var(const DualTrace& x, std::int64_t dim, bool unbiased, bool keepdim = false) {
  const Series primal = x.value().var(dim, unbiased, keepdim);
  const Series m = x.value().mean(dim, true);
  const Series c = x.value().sub(m);
  const Series dm = x.tangent().mean(dim, true);
  const Series dc = x.tangent().sub(dm);
  Series dvar = c.mul(dc).mean(dim, keepdim).mul(2.0);
  if (unbiased) {
    const auto rank = x.value().dim();
    const auto d = dim < 0 ? dim + rank : dim;
    const double n = static_cast<double>(x.value().sizes()[static_cast<std::size_t>(d)]);
    if (n <= 1.0) {
      ::pluss::throw_error("subsp::math::diff::var(unbiased): reduced dimension must have more than one element");
    }
    dvar = dvar.mul(n / (n - 1.0));
  }
  return DualTrace(primal, dvar);
}

// std = sqrt(var); d(std)/dx . v = d(var)/dx.v / (2*sqrt(var)).
inline Trace std(const Trace& x, std::int64_t dim, bool unbiased, bool keepdim = false) {
  detail::require_defined_(x.value(), "std");
  return Trace(x.value().std(dim, unbiased, keepdim));
}

inline DualTrace std(const DualTrace& x, std::int64_t dim, bool unbiased, bool keepdim = false) {
  const DualTrace v = var(x, dim, unbiased, keepdim);
  const Series primal = v.value().sqrt();
  const Series dstd = v.tangent().div(primal.mul(2.0));
  return DualTrace(primal, dstd);
}

// ---------------------------------------------------------------------------
// Track C batch 3: cumulative ops. cumsum is linear (same "apply the op to
// the tangent" rule as batch 1). cumprod is NOT linear and has a genuine
// singularity at zero elements (the naive quotient-rule form divides by
// x_k) -- mirrors the exact zero-safe construction PyTorch's own
// `cumprod_jvp` (torch/csrc/autograd/FunctionsManual.cpp) uses rather than
// inventing a formula: everywhere before the first zero (per reduced-dim
// slice) the quotient-rule form is exact and used directly; from the first
// zero onward, the correct value is a cumprod of (that position's own
// tangent substituted in) times the values after it.
// ---------------------------------------------------------------------------

inline Trace cumsum(const Trace& x, std::int64_t dim) {
  detail::require_defined_(x.value(), "cumsum");
  return Trace(x.value().cumsum(dim));
}

inline DualTrace cumsum(const DualTrace& x, std::int64_t dim) {
  return DualTrace(x.value().cumsum(dim), x.tangent().cumsum(dim));
}

inline Trace cumprod(const Trace& x, std::int64_t dim) {
  detail::require_defined_(x.value(), "cumprod");
  return Trace(x.value().cumprod(dim));
}

inline DualTrace cumprod(const DualTrace& x, std::int64_t dim) {
  const Series result = x.value().cumprod(dim);
  const Series generic_grad = x.tangent().div(x.value()).cumsum(dim).mul(result);

  const Series mask_zeros = x.value().eq(0.0);
  const Series mask_zeros_f = mask_zeros.to(x.value().scalar_type());
  const Series mask_first_zero = mask_zeros.logical_and(mask_zeros_f.cumsum(dim).eq(1.0));
  // Series::where(cond, other) selects `other` where cond is true and self
  // (the receiver) where false -- the opposite of torch's Tensor::where --
  // so "mask_first_zero ? tangent : value" is value.where(mask, tangent).
  const Series new_grad = x.value().where(mask_first_zero, x.tangent()).cumprod(dim);
  const Series mask_after_first_zero = mask_first_zero.to(x.value().scalar_type()).cumsum(dim).gt(0.0);
  const Series grad = generic_grad.where(mask_after_first_zero, new_grad);
  return DualTrace(result, grad);
}

// ---------------------------------------------------------------------------
// Track C batch 4: index/gather/scatter and pile (torch's `stack` -- pluss
// has no separate `cat`, see Series::pile in OpsInternal.cpp). All of these
// are linear: each output element is either a fixed input element or a
// fixed sum of input elements, entirely independent of the actual values,
// so "apply the same op to the tangent(s)" is exact here too. `index` is
// always a plain (non-differentiable) integer Series, never a DualTrace.
// ---------------------------------------------------------------------------

inline Trace index_select(const Trace& x, std::int64_t dim, const Series& index) {
  detail::require_defined_(x.value(), "index_select");
  return Trace(x.value().index_select(dim, index));
}

inline DualTrace index_select(const DualTrace& x, std::int64_t dim, const Series& index) {
  return DualTrace(x.value().index_select(dim, index), x.tangent().index_select(dim, index));
}

inline Trace gather(const Trace& x, std::int64_t dim, const Series& index) {
  detail::require_defined_(x.value(), "gather");
  return Trace(x.value().gather(dim, index));
}

inline DualTrace gather(const DualTrace& x, std::int64_t dim, const Series& index) {
  return DualTrace(x.value().gather(dim, index), x.tangent().gather(dim, index));
}

inline Trace scatter(const Trace& x, std::int64_t dim, const Series& index, const Trace& src) {
  detail::require_defined_(x.value(), "scatter");
  detail::require_defined_(src.value(), "scatter");
  return Trace(x.value().scatter(dim, index, src.value()));
}

inline DualTrace scatter(const DualTrace& x, std::int64_t dim, const Series& index, const DualTrace& src) {
  return DualTrace(
      x.value().scatter(dim, index, src.value()),
      x.tangent().scatter(dim, index, src.tangent()));
}

inline Trace scatter_add(const Trace& x, std::int64_t dim, const Series& index, const Trace& src) {
  detail::require_defined_(x.value(), "scatter_add");
  detail::require_defined_(src.value(), "scatter_add");
  return Trace(x.value().scatter_add(dim, index, src.value()));
}

inline DualTrace scatter_add(const DualTrace& x, std::int64_t dim, const Series& index, const DualTrace& src) {
  return DualTrace(
      x.value().scatter_add(dim, index, src.value()),
      x.tangent().scatter_add(dim, index, src.tangent()));
}

inline Trace pile(const std::vector<Trace>& xs, std::int64_t dim = 0) {
  if (xs.empty()) {
    ::pluss::throw_error("subsp::math::diff::pile: expected at least one input");
  }
  std::vector<Series> values;
  values.reserve(xs.size());
  for (const auto& x : xs) values.push_back(x.value());
  const std::vector<Series> others(values.begin() + 1, values.end());
  return Trace(values.front().pile(others, dim));
}

inline DualTrace pile(const std::vector<DualTrace>& xs, std::int64_t dim = 0) {
  if (xs.empty()) {
    ::pluss::throw_error("subsp::math::diff::pile: expected at least one input");
  }
  std::vector<Series> values;
  std::vector<Series> tangents;
  values.reserve(xs.size());
  tangents.reserve(xs.size());
  for (const auto& x : xs) {
    values.push_back(x.value());
    tangents.push_back(x.tangent());
  }
  const std::vector<Series> other_values(values.begin() + 1, values.end());
  const std::vector<Series> other_tangents(tangents.begin() + 1, tangents.end());
  return DualTrace(values.front().pile(other_values, dim), tangents.front().pile(other_tangents, dim));
}

inline Trace apply_loss_reduction(const Trace& x, LossReduction reduction) {
  switch (reduction) {
    case LossReduction::None:
      return x;
    case LossReduction::Sum:
      return sum(x);
    case LossReduction::Mean:
    default:
      return mean(x);
  }
}

inline DualTrace apply_loss_reduction(const DualTrace& x, LossReduction reduction) {
  switch (reduction) {
    case LossReduction::None:
      return x;
    case LossReduction::Sum:
      return sum(x);
    case LossReduction::Mean:
    default:
      return mean(x);
  }
}

inline Trace squared_error_loss(const Trace& prediction,
                                const Trace& target,
                                LossReduction reduction = LossReduction::Mean) {
  detail::require_defined_(prediction.value(), "squared_error_loss");
  detail::require_defined_(target.value(), "squared_error_loss");
  detail::require_same_platform_(prediction.value(), target.value(), "squared_error_loss");
  return apply_loss_reduction(square(sub(prediction, target)), reduction);
}

inline DualTrace squared_error_loss(const DualTrace& prediction,
                                    const DualTrace& target,
                                    LossReduction reduction = LossReduction::Mean) {
  return apply_loss_reduction(square(sub(prediction, target)), reduction);
}

inline Trace squared_error_loss(const Trace& prediction,
                                const Series& target,
                                LossReduction reduction = LossReduction::Mean) {
  return squared_error_loss(prediction, fixed(target), reduction);
}

inline DualTrace squared_error_loss(const DualTrace& prediction,
                                    const Series& target,
                                    LossReduction reduction = LossReduction::Mean) {
  return squared_error_loss(prediction, fixed(target, prediction.tangent()), reduction);
}

inline Trace absolute_error_loss(const Trace& prediction,
                                 const Trace& target,
                                 LossReduction reduction = LossReduction::Mean) {
  detail::require_defined_(prediction.value(), "absolute_error_loss");
  detail::require_defined_(target.value(), "absolute_error_loss");
  detail::require_same_platform_(prediction.value(), target.value(), "absolute_error_loss");
  return apply_loss_reduction(abs(sub(prediction, target)), reduction);
}

inline DualTrace absolute_error_loss(const DualTrace& prediction,
                                     const DualTrace& target,
                                     LossReduction reduction = LossReduction::Mean) {
  return apply_loss_reduction(abs(sub(prediction, target)), reduction);
}

inline Trace absolute_error_loss(const Trace& prediction,
                                 const Series& target,
                                 LossReduction reduction = LossReduction::Mean) {
  return absolute_error_loss(prediction, fixed(target), reduction);
}

inline DualTrace absolute_error_loss(const DualTrace& prediction,
                                     const Series& target,
                                     LossReduction reduction = LossReduction::Mean) {
  return absolute_error_loss(prediction, fixed(target, prediction.tangent()), reduction);
}

inline Trace huber_error_loss(const Trace& prediction,
                              const Trace& target,
                              double delta = 1.0,
                              LossReduction reduction = LossReduction::Mean) {
  if (!(delta > 0.0) || !std::isfinite(delta)) {
    ::pluss::throw_error("subsp::math::diff::huber_error_loss: delta must be positive and finite");
  }
  const Trace err = sub(prediction, target);
  const Trace a = abs(err);
  const Trace quadratic = clamp(a, 0.0, delta);
  const Trace linear = sub(a, quadratic);
  const Trace half = make_scalar_like_(quadratic, 0.5);
  const Trace delta_s = make_scalar_like_(quadratic, delta);
  const Trace loss = add(mul(half, square(quadratic)), mul(delta_s, linear));
  return apply_loss_reduction(loss, reduction);
}

inline DualTrace huber_error_loss(const DualTrace& prediction,
                                  const DualTrace& target,
                                  double delta = 1.0,
                                  LossReduction reduction = LossReduction::Mean) {
  if (!(delta > 0.0) || !std::isfinite(delta)) {
    ::pluss::throw_error("subsp::math::diff::huber_error_loss: delta must be positive and finite");
  }
  const DualTrace err = sub(prediction, target);
  const DualTrace a = abs(err);
  const DualTrace quadratic = clamp(a, 0.0, delta);
  const DualTrace linear = sub(a, quadratic);
  const DualTrace half = make_scalar_like_(quadratic, 0.5);
  const DualTrace delta_s = make_scalar_like_(quadratic, delta);
  const DualTrace loss = add(mul(half, square(quadratic)), mul(delta_s, linear));
  return apply_loss_reduction(loss, reduction);
}

inline Trace huber_error_loss(const Trace& prediction,
                              const Series& target,
                              double delta = 1.0,
                              LossReduction reduction = LossReduction::Mean) {
  return huber_error_loss(prediction, fixed(target), delta, reduction);
}

inline DualTrace huber_error_loss(const DualTrace& prediction,
                                  const Series& target,
                                  double delta = 1.0,
                                  LossReduction reduction = LossReduction::Mean) {
  return huber_error_loss(prediction, fixed(target, prediction.tangent()), delta, reduction);
}

inline Trace logistic_binary_loss(const Trace& logits,
                                  const Trace& target,
                                  LossReduction reduction = LossReduction::Mean) {
  detail::require_defined_(logits.value(), "logistic_binary_loss");
  detail::require_defined_(target.value(), "logistic_binary_loss");
  detail::require_same_platform_(logits.value(), target.value(), "logistic_binary_loss");
  const Trace one = make_scalar_like_(target, 1.0);
  const Trace one_minus_t = sub(one, target);
  const Trace terms = add(mul(one_minus_t, softplus(logits)),
                          mul(target, softplus(neg(logits))));
  return apply_loss_reduction(terms, reduction);
}

inline DualTrace logistic_binary_loss(const DualTrace& logits,
                                      const DualTrace& target,
                                      LossReduction reduction = LossReduction::Mean) {
  const DualTrace one = make_scalar_like_(target, 1.0);
  const DualTrace one_minus_t = sub(one, target);
  const DualTrace terms = add(mul(one_minus_t, softplus(logits)),
                              mul(target, softplus(neg(logits))));
  return apply_loss_reduction(terms, reduction);
}

inline Trace logistic_binary_loss(const Trace& logits,
                                  const Series& target,
                                  LossReduction reduction = LossReduction::Mean) {
  return logistic_binary_loss(logits, fixed(target), reduction);
}

inline DualTrace logistic_binary_loss(const DualTrace& logits,
                                      const Series& target,
                                      LossReduction reduction = LossReduction::Mean) {
  return logistic_binary_loss(logits, fixed(target, logits.tangent()), reduction);
}

inline Trace matmul(const Trace& a, const Trace& b) {
  detail::require_defined_(a.value(), "matmul");
  detail::require_defined_(b.value(), "matmul");
  detail::require_same_platform_(a.value(), b.value(), "matmul");
  if (a.value().dim() != 2 || b.value().dim() != 2) {
    ::pluss::throw_error("subsp::math::diff::matmul: expected 2D inputs");
  }
  return Trace(a.value().matmul(b.value()));
}

inline DualTrace matmul(const DualTrace& a, const DualTrace& b) {
  if (a.value().dim() != 2 || b.value().dim() != 2) {
    ::pluss::throw_error("subsp::math::diff::matmul: expected 2D inputs");
  }
  return DualTrace(
      a.value().matmul(b.value()),
      a.tangent().matmul(b.value()).add(a.value().matmul(b.tangent())));
}

// pluss DOES have a `cat`-equivalent after all -- Series::unite(others, dim)
// (torch's `cat`, joins along an EXISTING dim) -- this batch's `pile` note
// above ("pluss has no cat, only pile/stack") was wrong; correcting it here
// rather than leaving it stand. Same linearity as pile.
inline Trace unite(const std::vector<Trace>& xs, std::int64_t dim = 0) {
  if (xs.empty()) {
    ::pluss::throw_error("subsp::math::diff::unite: expected at least one input");
  }
  std::vector<Series> values;
  values.reserve(xs.size());
  for (const auto& x : xs) values.push_back(x.value());
  const std::vector<Series> others(values.begin() + 1, values.end());
  return Trace(values.front().unite(others, dim));
}

inline DualTrace unite(const std::vector<DualTrace>& xs, std::int64_t dim = 0) {
  if (xs.empty()) {
    ::pluss::throw_error("subsp::math::diff::unite: expected at least one input");
  }
  std::vector<Series> values;
  std::vector<Series> tangents;
  values.reserve(xs.size());
  tangents.reserve(xs.size());
  for (const auto& x : xs) {
    values.push_back(x.value());
    tangents.push_back(x.tangent());
  }
  const std::vector<Series> other_values(values.begin() + 1, values.end());
  const std::vector<Series> other_tangents(tangents.begin() + 1, tangents.end());
  return DualTrace(values.front().unite(other_values, dim), tangents.front().unite(other_tangents, dim));
}

// ---------------------------------------------------------------------------
// Track C batch 6: the linalg family. Scoped to real (non-complex) matrices,
// matching the scope the reverse-mode linalg oracle tests already use for
// this same op family (det/logdet/inverse/pinverse/cholesky/matrix_exp/
// matrix_power in F64/F32; QR/SVD's complex coverage was a separate,
// narrower reverse-mode test). Formulas ported from PyTorch's own
// FunctionsManual.cpp (`cholesky_jvp`, `differential_analytic_matrix_
// function`, `pinv_jvp`, `linalg_qr_jvp`) rather than re-derived, per the
// project's "check against local torch" standard -- these are exactly the
// formulas that file already documents, translated to Series calls.
// ---------------------------------------------------------------------------

inline Trace det(const Trace& x) {
  detail::require_defined_(x.value(), "det");
  return Trace(pluss::subsp::linearalg::det(x.value()));
}

inline DualTrace det(const DualTrace& x) {
  const Series primal = pluss::subsp::linearalg::det(x.value());
  const Series inv = pluss::subsp::linearalg::inverse(x.value());
  const Series trace_term = inv.matmul(x.tangent()).diagonal(0, -2, -1).sum(-1, false);
  return DualTrace(primal, trace_term.mul(primal));
}

inline Trace logdet(const Trace& x) {
  detail::require_defined_(x.value(), "logdet");
  return Trace(pluss::subsp::linearalg::logdet(x.value()));
}

inline DualTrace logdet(const DualTrace& x) {
  const Series primal = pluss::subsp::linearalg::logdet(x.value());
  const Series inv = pluss::subsp::linearalg::inverse(x.value());
  const Series trace_term = inv.matmul(x.tangent()).diagonal(0, -2, -1).sum(-1, false);
  return DualTrace(primal, trace_term);
}

// slogdet's packed [..., 2] output is [sign, logabsdet] (see
// bind_slogdet_backward_ in LinearAlgFunctional.cpp for the same
// convention). d(sign) = 0 for real matrices (sign is locally constant
// a.e.); d(logabsdet) = tr(A^{-1} dA), same as logdet.
inline Trace slogdet(const Trace& x) {
  detail::require_defined_(x.value(), "slogdet");
  return Trace(pluss::subsp::linearalg::slogdet(x.value()));
}

inline DualTrace slogdet(const DualTrace& x) {
  const Series primal = pluss::subsp::linearalg::slogdet(x.value());
  const Series inv = pluss::subsp::linearalg::inverse(x.value());
  const Series trace_term = inv.matmul(x.tangent()).diagonal(0, -2, -1).sum(-1, false);
  const Series dsign = detail::zeros_like_(trace_term);
  const Series tangent = dsign.pile(std::vector<Series>{trace_term}, -1);
  return DualTrace(primal, tangent);
}

inline Trace inverse(const Trace& x) {
  detail::require_defined_(x.value(), "inverse");
  return Trace(pluss::subsp::linearalg::inverse(x.value()));
}

inline DualTrace inverse(const DualTrace& x) {
  const Series primal = pluss::subsp::linearalg::inverse(x.value());
  const Series tangent = primal.matmul(x.tangent()).matmul(primal).neg();
  return DualTrace(primal, tangent);
}

inline Trace cholesky(const Trace& x, bool upper = false) {
  detail::require_defined_(x.value(), "cholesky");
  return Trace(pluss::subsp::linearalg::cholesky(x.value(), upper));
}

inline DualTrace cholesky(const DualTrace& x, bool upper = false) {
  const Series L = pluss::subsp::linearalg::cholesky(x.value(), upper);
  const Series L_ = upper ? L.mH() : L;
  Series dL = pluss::subsp::linearalg::triangular_solve(L_, x.tangent(), /*upper=*/false, false);
  dL = pluss::subsp::linearalg::triangular_solve(L_, dL.mH(), /*upper=*/false, false).mH();
  const Series half_diag = dL.diagonal(0, -2, -1).mul(0.5).diag_embed();
  dL = dL.tril().sub(half_diag);
  dL = L_.matmul(dL);
  return DualTrace(L, upper ? dL.mH() : dL);
}

inline Trace matrix_exp(const Trace& x) {
  detail::require_defined_(x.value(), "matrix_exp");
  return Trace(pluss::subsp::linearalg::matrix_exp(x.value()));
}

inline DualTrace matrix_exp(const DualTrace& x) {
  const Series& A = x.value();
  const Series primal = pluss::subsp::linearalg::matrix_exp(A);
  // Higham's classic trick (ported from torch's
  // differential_analytic_matrix_function): the JVP of any analytic matrix
  // function f at A in direction dA is the top-right block of
  // f([[A, dA], [0, A]]).
  const Series zero_block = detail::zeros_like_(A);
  const Series top = A.unite(std::vector<Series>{x.tangent()}, -1);
  const Series bottom = zero_block.unite(std::vector<Series>{A}, -1);
  const Series block = top.unite(std::vector<Series>{bottom}, -2);
  const Series block_exp = pluss::subsp::linearalg::matrix_exp(block);
  const auto rank = A.dim();
  const auto n = A.sizes()[static_cast<std::size_t>(rank - 1)];
  const Series tangent = block_exp.slice(rank - 2, 0, n).slice(rank - 1, n, 2 * n);
  return DualTrace(primal, tangent);
}

inline Trace matrix_power(const Trace& x, std::int64_t n) {
  detail::require_defined_(x.value(), "matrix_power");
  return Trace(pluss::subsp::linearalg::matrix_power(x.value(), n));
}

inline DualTrace matrix_power(const DualTrace& x, std::int64_t n) {
  const Series primal = pluss::subsp::linearalg::matrix_power(x.value(), n);
  if (n == 0) {
    return DualTrace(primal, detail::zeros_like_(x.tangent()));
  }
  const bool neg = n < 0;
  const Series base = neg ? pluss::subsp::linearalg::inverse(x.value()) : x.value();
  const Series dbase = neg ? base.matmul(x.tangent()).matmul(base).neg() : x.tangent();
  const std::int64_t k = neg ? -n : n;
  Series acc = pluss::series::undefined();
  for (std::int64_t i = 0; i < k; ++i) {
    const Series left_pow = pluss::subsp::linearalg::matrix_power(base, i);
    const Series right_pow = pluss::subsp::linearalg::matrix_power(base, k - 1 - i);
    const Series term = left_pow.matmul(dbase).matmul(right_pow);
    acc = (i == 0) ? term : acc.add(term);
  }
  return DualTrace(primal, acc);
}

// Direct (non-shape-optimized) form of Golub & Pereyra's pseudoinverse JVP,
// ported from torch's pinv_jvp -- that function branches on m<=n vs m>n
// purely to shrink an intermediate matrix for performance; both branches
// are algebraically equivalent, so this uses the single unbranched formula.
inline Trace pinverse(const Trace& x, double rcond = 1e-12) {
  detail::require_defined_(x.value(), "pinverse");
  return Trace(pluss::subsp::linearalg::pinverse(x.value(), rcond));
}

inline DualTrace pinverse(const DualTrace& x, double rcond = 1e-12) {
  const Series& A = x.value();
  const Series Ap = pluss::subsp::linearalg::pinverse(A, rcond);
  const Series& dA = x.tangent();
  const Series dAh = dA.mH();
  const Series Aph = Ap.mH();
  const Series ApA = Ap.matmul(A);
  const Series AAp = A.matmul(Ap);
  const Series I_n = pluss::subsp::linearalg::matrix_power(ApA, 0);
  const Series I_m = pluss::subsp::linearalg::matrix_power(AAp, 0);
  const Series term1 = Ap.matmul(dA).matmul(Ap).neg();
  const Series term2 = I_n.sub(ApA).matmul(dAh).matmul(Aph).matmul(Ap);
  const Series term3 = Ap.matmul(Aph).matmul(dAh).matmul(I_m.sub(AAp));
  const Series dAp = term1.add(term2).add(term3);
  return DualTrace(Ap, dAp);
}

inline Trace triangular_solve(const Trace& a, const Trace& b, bool upper = true, bool unitriangular = false) {
  detail::require_defined_(a.value(), "triangular_solve");
  detail::require_defined_(b.value(), "triangular_solve");
  return Trace(pluss::subsp::linearalg::triangular_solve(a.value(), b.value(), upper, unitriangular));
}

inline DualTrace triangular_solve(
    const DualTrace& a, const DualTrace& b, bool upper = true, bool unitriangular = false) {
  const Series X = pluss::subsp::linearalg::triangular_solve(a.value(), b.value(), upper, unitriangular);
  const Series rhs = b.tangent().sub(a.tangent().matmul(X));
  const Series dX = pluss::subsp::linearalg::triangular_solve(a.value(), rhs, upper, unitriangular);
  return DualTrace(X, dX);
}

inline Trace qr_q(const Trace& x, bool reduced = true) {
  detail::require_defined_(x.value(), "qr_q");
  return Trace(pluss::subsp::linearalg::qr_q(x.value(), reduced));
}

inline DualTrace qr_q(const DualTrace& x, bool reduced = true) {
  const Series Q = pluss::subsp::linearalg::qr_q(x.value(), reduced);
  const Series R = pluss::subsp::linearalg::qr_r(x.value(), reduced);
  const auto jvp_pair = detail::qr_jvp_(Q, R, x.tangent());
  return DualTrace(Q, jvp_pair.first);
}

inline Trace qr_r(const Trace& x, bool reduced = true) {
  detail::require_defined_(x.value(), "qr_r");
  return Trace(pluss::subsp::linearalg::qr_r(x.value(), reduced));
}

inline DualTrace qr_r(const DualTrace& x, bool reduced = true) {
  const Series Q = pluss::subsp::linearalg::qr_q(x.value(), reduced);
  const Series R = pluss::subsp::linearalg::qr_r(x.value(), reduced);
  const auto jvp_pair = detail::qr_jvp_(Q, R, x.tangent());
  return DualTrace(R, jvp_pair.second);
}

// Singular VALUES only -- U/V columns have gauge freedom under repeated
// singular values with no clean closed form, so (matching the scope the
// reverse-mode SVD oracle test already settled on for this same reason) only
// dS is implemented. dS_i = Re(u_i^H dA v_i), which for real matrices is
// just diagonal(U^H dA V).
inline Trace svd_s(const Trace& x, bool full_matrices = false) {
  detail::require_defined_(x.value(), "svd_s");
  return Trace(pluss::subsp::linearalg::svd_s(x.value(), full_matrices));
}

inline DualTrace svd_s(const DualTrace& x, bool full_matrices = false) {
  const auto decomp = pluss::subsp::linearalg::svd(x.value(), full_matrices);
  const Series UhdAV = decomp.u.mH().matmul(x.tangent()).matmul(decomp.vh.mH());
  return DualTrace(decomp.s, UhdAV.diagonal(0, -2, -1));
}

inline Trace operator+(const Trace& a, const Trace& b) { return add(a, b); }
inline Trace operator-(const Trace& a, const Trace& b) { return sub(a, b); }
inline Trace operator*(const Trace& a, const Trace& b) { return mul(a, b); }
inline Trace operator/(const Trace& a, const Trace& b) { return div(a, b); }

inline DualTrace operator+(const DualTrace& a, const DualTrace& b) { return add(a, b); }
inline DualTrace operator-(const DualTrace& a, const DualTrace& b) { return sub(a, b); }
inline DualTrace operator*(const DualTrace& a, const DualTrace& b) { return mul(a, b); }
inline DualTrace operator/(const DualTrace& a, const DualTrace& b) { return div(a, b); }

inline Trace operator+(const Trace& a, double s) { return add(a, make_scalar_like_(a, s)); }
inline Trace operator-(const Trace& a, double s) { return sub(a, make_scalar_like_(a, s)); }
inline Trace operator*(const Trace& a, double s) { return mul(a, make_scalar_like_(a, s)); }
inline Trace operator/(const Trace& a, double s) { return div(a, make_scalar_like_(a, s)); }

inline DualTrace operator+(const DualTrace& a, double s) { return add(a, make_scalar_like_(a, s)); }
inline DualTrace operator-(const DualTrace& a, double s) { return sub(a, make_scalar_like_(a, s)); }
inline DualTrace operator*(const DualTrace& a, double s) { return mul(a, make_scalar_like_(a, s)); }
inline DualTrace operator/(const DualTrace& a, double s) { return div(a, make_scalar_like_(a, s)); }

inline Trace operator+(double s, const Trace& a) { return add(make_scalar_like_(a, s), a); }
inline Trace operator-(double s, const Trace& a) { return sub(make_scalar_like_(a, s), a); }
inline Trace operator*(double s, const Trace& a) { return mul(make_scalar_like_(a, s), a); }
inline Trace operator/(double s, const Trace& a) { return div(make_scalar_like_(a, s), a); }

inline DualTrace operator+(double s, const DualTrace& a) { return add(make_scalar_like_(a, s), a); }
inline DualTrace operator-(double s, const DualTrace& a) { return sub(make_scalar_like_(a, s), a); }
inline DualTrace operator*(double s, const DualTrace& a) { return mul(make_scalar_like_(a, s), a); }
inline DualTrace operator/(double s, const DualTrace& a) { return div(make_scalar_like_(a, s), a); }

inline Trace operator-(const Trace& x) { return neg(x); }
inline DualTrace operator-(const DualTrace& x) { return neg(x); }

inline void clear_sensitivities(std::vector<Trace>& params) {
  for (auto& p : params) {
    p.clear();
  }
}

template <class Fn>
std::pair<Series, std::function<std::vector<Series>(const Series&)>> vjp(
    Fn&& fn, const std::vector<Series>& params, const DerivativePolicy& policy);

template <class Fn>
std::pair<Series, std::vector<Series>> vjp(
    Fn&& fn,
    const std::vector<Series>& params,
    const Series& cotangent,
    const DerivativePolicy& policy = {});

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> vjp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<Series>& cotangent_lanes,
    const DerivativePolicy& policy = {});

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> hvp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& tangent_lanes,
    const HvpOptions& opt = {},
    const DerivativePolicy& policy = {});

template <class Fn>
std::pair<Series, std::vector<Series>> vhp(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<Series>& vectors,
    const HvpOptions& opt = {},
    const DerivativePolicy& policy = {});

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> vhp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& vector_lanes,
    const HvpOptions& opt = {},
    const DerivativePolicy& policy = {});

template <class Fn>
bool gradient_audit(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt = {});

template <class Fn>
bool curvature_audit(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt = {});

template <class Fn>
DerivativeAuditReport gradient_audit_report(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt = {});

template <class Fn>
DerivativeAuditReport curvature_audit_report(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt = {});

namespace detail {

// A derivative transform accepts a callback written over Trace or over plain Series: a Series callback is
// called with the (slope-tracking) values themselves and its result is wrapped back into a Trace.
template <class Fn>
Trace call_traced_(Fn&& fn, const std::vector<Trace>& vars) {
  if constexpr (std::invocable<Fn&, const std::vector<Trace>&>) {
    return fn(vars);
  } else {
    std::vector<Series> values;
    values.reserve(vars.size());
    for (const auto& v : vars) values.push_back(v.value());
    return Trace(fn(values));
  }
}

template <class Fn>
Trace call_traced_(Fn&& fn, const Trace& var) {
  if constexpr (std::invocable<Fn&, const Trace&>) {
    return fn(var);
  } else {
    return Trace(fn(var.value()));
  }
}

}  // namespace detail

template <class Fn>
DerivativeAuditReport gradient_audit_report(
    Fn&& fn,
    const Series& param,
    const DerivativeAuditOptions& opt = {});

template <class Fn>
DerivativeAuditReport curvature_audit_report(
    Fn&& fn,
    const Series& param,
    const DerivativeAuditOptions& opt = {});

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_grad(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativePolicy& policy = {}) {
  std::vector<Trace> vars;
  vars.reserve(params.size());
  for (const auto& p : params) {
    vars.emplace_back(p);
    detail::ensure_tracks_slope_(vars.back().value());
  }
  detail::SensitivityRetentionScope retain_inputs(params);

  Trace loss = detail::call_traced_(std::forward<Fn>(fn), vars);
  if (!loss.defined()) {
    ::pluss::throw_error("subsp::math::diff::value_and_grad: objective returned undefined Trace");
  }

  if (!policy.accumulate_input_sensitivity) {
    clear_sensitivities(vars);
  }
  ReversePolicy reverse_policy{};
  reverse_policy.keep_graph = policy.keep_graph;
  reverse_policy.accumulate_sensitivity = policy.accumulate_input_sensitivity;
  reverse_policy.track_higher_order = policy.track_higher_order;
  if (detail::has_reverse_path_(loss.value())) {
    loss.reverse(reverse_policy);
  }

  std::vector<Series> grads;
  grads.reserve(vars.size());
  bool disconnected_warned = false;
  for (std::size_t i = 0; i < vars.size(); ++i) {
    const auto& v = vars[i];
    if (!v.has_sensitivity()) {
      if (policy.strict_dependency) {
        ::pluss::throw_error(
            "subsp::math::diff::value_and_grad: objective is disconnected from input index " +
            std::to_string(i));
      }
      if (policy.warn_on_disconnected && !disconnected_warned) {
        PLUSS_WARN(
            "subsp::math::diff::value_and_grad: objective disconnected from one or more inputs; "
            "materializing disconnected sensitivities according to policy "
            "(set strict_dependency=true to fail loud).");
        disconnected_warned = true;
      }
      if (policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined) {
        grads.push_back(pluss::series::undefined());
        continue;
      }
      grads.push_back(detail::zeros_like_(v.value()));
      continue;
    }
    grads.push_back(v.sensitivity());
  }
  // A one-element value is returned as a rank-zero Series whatever shape the callback gave it.
  Series value = loss.value();
  if (value.numel() == 1 && value.dim() != 0) value = value.reshape(std::vector<std::int64_t>{});
  return {value, grads};
}

template <class Fn>
std::vector<Series> grad(Fn&& fn,
                         const std::vector<Series>& params,
                         const DerivativePolicy& policy = {}) {
  return value_and_grad(std::forward<Fn>(fn), params, policy).second;
}

template <class Fn>
std::pair<Series, std::vector<Series>> value_and_grad(
    Fn&& fn,
    const std::vector<Series>& params,
    const Series& cotangent,
    const DerivativePolicy& policy = {}) {
  auto [value, pullback] = vjp(std::forward<Fn>(fn), params, policy);
  return {value, pullback(cotangent)};
}

template <class Fn>
std::vector<Series> grad(Fn&& fn,
                         const std::vector<Series>& params,
                         const Series& cotangent,
                         const DerivativePolicy& policy = {}) {
  return value_and_grad(std::forward<Fn>(fn), params, cotangent, policy).second;
}

template <class Fn>
std::pair<Series, std::vector<Series>> jacrev(Fn&& fn,
                                              const std::vector<Series>& params,
                                              const DerivativePolicy& policy = {}) {
  // Reverse-mode Jacobian requires repeated pullbacks from one captured graph.
  // Force graph retention internally regardless of user one-shot policy.
  DerivativePolicy jac_policy = policy;
  jac_policy.keep_graph = true;
  auto [value, pullback] = vjp(std::forward<Fn>(fn), params, jac_policy);
  const std::int64_t output_numel = value.numel();

  std::vector<std::vector<Series>> rows_by_param(params.size());
  for (auto& rows : rows_by_param) rows.reserve(static_cast<std::size_t>(output_numel));

  for (std::int64_t row = 0; row < output_numel; ++row) {
    const Series cotangent = detail::flat_basis_like_(value, row);
    const auto grads = pullback(cotangent);
    for (std::size_t p = 0; p < grads.size(); ++p) {
      rows_by_param[p].push_back(grads[p].reshape({1, grads[p].numel()}));
    }
  }

  std::vector<Series> jacobians;
  jacobians.reserve(params.size());
  for (std::size_t p = 0; p < params.size(); ++p) {
    jacobians.push_back(detail::stack_rows_(
        rows_by_param[p], params[p].numel(), params[p].scalar_type(), params[p].platform()));
  }

  return {value, jacobians};
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn,
                              const std::vector<Series>& params,
                              const std::vector<Series>& tangents) {
  if (params.size() != tangents.size()) {
    ::pluss::throw_error("subsp::math::diff::jvp: params/tangents size mismatch");
  }

  using FnType = std::decay_t<Fn>;
  if constexpr (!std::is_invocable_v<FnType&, const std::vector<DualTrace>&>) {
    // A callback over plain Series: J t from the reverse-mode Jacobian, one pullback per output element.
    auto [value, jacobians] = jacrev(std::forward<Fn>(fn), params);
    Series directional = pluss::series::undefined();
    for (std::size_t i = 0; i < params.size(); ++i) {
      const Series t = tangents[i].reshape({tangents[i].numel(), 1});
      const Series part = jacobians[i].matmul(t).reshape({jacobians[i].size(0)});
      directional = directional.defined() ? directional.add(part) : part;
    }
    if (!directional.defined()) directional = detail::zeros_like_(value).reshape({value.numel()});
    return {value, directional.reshape(std::vector<std::int64_t>(value.sizes().begin(), value.sizes().end()))};
  } else {
    std::vector<DualTrace> vars;
    vars.reserve(params.size());
    for (std::size_t i = 0; i < params.size(); ++i) {
      vars.emplace_back(params[i], tangents[i]);
    }

    DualTrace output = std::forward<Fn>(fn)(vars);
    if (!output.defined()) {
      ::pluss::throw_error("subsp::math::diff::jvp: objective returned undefined DualTrace");
    }

    return {output.value(), output.tangent()};
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> jvp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& tangent_lanes) {
  std::vector<Series> lanes;
  lanes.reserve(tangent_lanes.size());
  Series value = pluss::series::undefined();
  bool value_set = false;

  for (const auto& lane : tangent_lanes) {
    const auto [lane_value, lane_tangent] = jvp(fn, params, lane);
    if (!value_set) {
      value = lane_value;
      value_set = true;
    } else {
      detail::require_same_platform_(value, lane_value, "jvp_lanes");
      detail::require_same_shape_(value, lane_value, "jvp_lanes");
    }
    lanes.push_back(lane_tangent);
  }

  if (!value_set) {
    ::pluss::throw_error(
        "subsp::math::diff::jvp_lanes: expected at least one tangent lane");
  }
  return {value, lanes};
}

template <class Fn>
auto linearize(Fn&& fn, const std::vector<Series>& params) {
  using FnType = std::decay_t<Fn>;

  if constexpr (!std::is_invocable_v<FnType&, const std::vector<DualTrace>&>) {
    // Plain-Series callback: the Jacobian is built once by reverse mode and the pushforward is J t.
    auto [value, jacobians] = jacrev(std::forward<Fn>(fn), params);
    const std::vector<std::int64_t> value_shape(value.sizes().begin(), value.sizes().end());
    auto pushforward = [jacobians = std::move(jacobians), value_shape](const std::vector<Series>& tangents) -> Series {
      Series directional = pluss::series::undefined();
      for (std::size_t i = 0; i < jacobians.size(); ++i) {
        const Series t = tangents[i].reshape({tangents[i].numel(), 1});
        const Series part = jacobians[i].matmul(t).reshape({jacobians[i].size(0)});
        directional = directional.defined() ? directional.add(part) : part;
      }
      return directional.reshape(value_shape);
    };
    return std::pair{value, std::move(pushforward)};
  } else {
    std::vector<DualTrace> base_vars;
    base_vars.reserve(params.size());
    for (const auto& p : params) {
      detail::require_defined_(p, "linearize");
      base_vars.emplace_back(p, detail::zeros_like_(p));
    }

    FnType fn_copy(std::forward<Fn>(fn));
    DualTrace output = fn_copy(base_vars);
    if (!output.defined()) {
      ::pluss::throw_error("subsp::math::diff::linearize: objective returned undefined DualTrace");
    }

    auto pushforward = [fn = std::move(fn_copy), params](const std::vector<Series>& tangents) -> Series {
      return jvp(
          [&](const std::vector<DualTrace>& xs) -> DualTrace {
            return fn(xs);
          },
          params,
          tangents).second;
    };

    return std::pair{output.value(), std::move(pushforward)};
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> jacfwd(Fn&& fn, const std::vector<Series>& params) {
  if constexpr (!std::is_invocable_v<std::decay_t<Fn>&, const std::vector<DualTrace>&>) {
    return jacrev(std::forward<Fn>(fn), params);
  }
  auto [value, pushforward] = linearize(std::forward<Fn>(fn), params);
  const std::int64_t output_numel = value.numel();

  std::vector<std::vector<Series>> cols_by_param(params.size());
  for (auto& cols : cols_by_param) cols.reserve(static_cast<std::size_t>(params.empty() ? 0 : params.front().numel()));

  for (std::size_t p = 0; p < params.size(); ++p) {
    cols_by_param[p].reserve(static_cast<std::size_t>(params[p].numel()));
    for (std::int64_t col = 0; col < params[p].numel(); ++col) {
      std::vector<Series> tangents;
      tangents.reserve(params.size());
      for (std::size_t q = 0; q < params.size(); ++q) {
        tangents.push_back(q == p ? detail::flat_basis_like_(params[q], col)
                                  : detail::zeros_like_(params[q]));
      }
      const Series directional = pushforward(tangents);
      cols_by_param[p].push_back(directional.reshape({directional.numel(), 1}));
    }
  }

  std::vector<Series> jacobians;
  jacobians.reserve(params.size());
  for (std::size_t p = 0; p < params.size(); ++p) {
    jacobians.push_back(detail::stack_cols_(
        cols_by_param[p], output_numel, value.scalar_type(), value.platform()));
  }

  return {value, jacobians};
}

template <class Fn>
std::pair<Series, std::function<std::vector<Series>(const Series&)>> vjp(
    Fn&& fn, const std::vector<Series>& params, const DerivativePolicy& policy) {
  std::vector<Trace> vars;
  vars.reserve(params.size());
  for (const auto& p : params) {
    vars.emplace_back(p);
    detail::ensure_tracks_slope_(vars.back().value());
  }

  Trace output = detail::call_traced_(std::forward<Fn>(fn), vars);
  if (!output.defined()) {
    ::pluss::throw_error("subsp::math::diff::vjp: objective returned undefined Trace");
  }

  auto pullback = [vars, output, policy, pullback_consumed = false](const Series& cotangent) mutable -> std::vector<Series> {
    if (!policy.keep_graph && pullback_consumed) {
      ::pluss::throw_error(
          "subsp::math::diff::vjp: pullback is one-shot when keep_graph=false; "
          "set keep_graph=true for reusable pullbacks");
    }
    detail::require_defined_(cotangent, "vjp");
    detail::require_same_platform_(output.value(), cotangent, "vjp");
    if (!detail::same_shape_(output.value(), cotangent)) {
      ::pluss::throw_error("subsp::math::diff::vjp: cotangent shape must match output shape");
    }

    if (!policy.accumulate_input_sensitivity) {
      clear_sensitivities(vars);
    }
    ReversePolicy reverse_policy{};
    reverse_policy.keep_graph = policy.keep_graph;
    reverse_policy.accumulate_sensitivity = policy.accumulate_input_sensitivity;
    reverse_policy.track_higher_order = policy.track_higher_order;
    if (detail::has_reverse_path_(output.value())) {
      output.reverse(cotangent, reverse_policy);
    }
    pullback_consumed = true;

    std::vector<Series> grads;
    grads.reserve(vars.size());
    bool disconnected_warned = false;
    for (std::size_t i = 0; i < vars.size(); ++i) {
      const auto& v = vars[i];
      if (!v.has_sensitivity()) {
        if (policy.strict_dependency) {
          ::pluss::throw_error(
              "subsp::math::diff::vjp: output is disconnected from input index " +
              std::to_string(i));
        }
        if (policy.warn_on_disconnected && !disconnected_warned) {
          PLUSS_WARN(
              "subsp::math::diff::vjp: output disconnected from one or more inputs; "
              "materializing disconnected sensitivities according to policy "
              "(set strict_dependency=true to fail loud).");
          disconnected_warned = true;
        }
        if (policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined) {
          grads.push_back(pluss::series::undefined());
          continue;
        }
        grads.push_back(detail::zeros_like_(v.value()));
        continue;
      }
      grads.push_back(v.sensitivity());
    }
    return grads;
  };

  return {output.value(), std::move(pullback)};
}

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> vjp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<Series>& cotangent_lanes,
    const DerivativePolicy& policy) {
  // Multi-lane pullbacks require repeated reverse passes from one captured graph.
  // Force retention internally regardless of user one-shot policy.
  DerivativePolicy lane_policy = policy;
  if (cotangent_lanes.size() > 1) {
    lane_policy.keep_graph = true;
  }
  auto [value, pullback] = vjp(std::forward<Fn>(fn), params, lane_policy);
  std::vector<std::vector<Series>> lanes;
  lanes.reserve(cotangent_lanes.size());
  for (const auto& lane : cotangent_lanes) {
    lanes.push_back(pullback(lane));
  }
  return {value, lanes};
}

template <class Fn>
std::pair<Series, std::vector<Series>> vjp(
    Fn&& fn,
    const std::vector<Series>& params,
    const Series& cotangent,
    const DerivativePolicy& policy) {
  auto [value, pullback] = vjp(std::forward<Fn>(fn), params, policy);
  return {value, pullback(cotangent)};
}

template <class Fn>
std::pair<Series, Series> value_and_grad(Fn&& fn,
                                         const Series& param,
                                         const DerivativePolicy& policy = {}) {
  std::vector<Series> params{param};
  auto [value, grads] = value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace {
        return detail::call_traced_(std::forward<Fn>(fn), xs.front());
      },
      params,
      policy);
  return {value, grads.front()};
}

template <class Fn>
Series grad(Fn&& fn, const Series& param, const DerivativePolicy& policy = {}) {
  return value_and_grad(std::forward<Fn>(fn), param, policy).second;
}

template <class Fn>
std::pair<Series, Series> value_and_grad(Fn&& fn,
                                         const Series& param,
                                         const Series& cotangent,
                                         const DerivativePolicy& policy = {}) {
  std::vector<Series> params{param};
  auto [value, grads] = value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace {
        return detail::call_traced_(std::forward<Fn>(fn), xs.front());
      },
      params,
      cotangent,
      policy);
  return {value, grads.front()};
}

template <class Fn>
Series grad(Fn&& fn,
            const Series& param,
            const Series& cotangent,
            const DerivativePolicy& policy = {}) {
  return value_and_grad(std::forward<Fn>(fn), param, cotangent, policy).second;
}

template <class Fn>
std::pair<Series, Series> jacrev(Fn&& fn,
                                 const Series& param,
                                 const DerivativePolicy& policy = {}) {
  auto [value, jacobians] = jacrev(
      [&](const std::vector<Trace>& xs) -> Trace {
        return detail::call_traced_(std::forward<Fn>(fn), xs.front());
      },
      std::vector<Series>{param},
      policy);
  return {value, jacobians.front()};
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn, const Series& param, const Series& tangent) {
  if constexpr (std::is_invocable_v<std::decay_t<Fn>&, const DualTrace&>) {
    return jvp(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return std::forward<Fn>(fn)(xs.front());
        },
        std::vector<Series>{param},
        std::vector<Series>{tangent});
  } else {
    return jvp(
        [&](const std::vector<Trace>& xs) -> Trace {
          return detail::call_traced_(std::forward<Fn>(fn), xs.front());
        },
        std::vector<Series>{param},
        std::vector<Series>{tangent});
  }
}

template <class Fn>
std::pair<Series, Series> jacfwd(Fn&& fn, const Series& param) {
  if constexpr (std::is_invocable_v<std::decay_t<Fn>&, const DualTrace&>) {
    auto [value, jacobians] = jacfwd(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return std::forward<Fn>(fn)(xs.front());
        },
        std::vector<Series>{param});
    return {value, jacobians.front()};
  } else {
    auto [value, jacobians] = jacrev(
        [&](const std::vector<Trace>& xs) -> Trace {
          return detail::call_traced_(std::forward<Fn>(fn), xs.front());
        },
        std::vector<Series>{param});
    return {value, jacobians.front()};
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> jacobian(
    Fn&& fn,
    const std::vector<Series>& params,
    const JacobianOptions& opt = {}) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  if (opt.mode == JacobianMode::Forward) {
    if (opt.policy.strict_dependency) {
      // Validate dependency semantics using reverse audit, then return forward Jacobian.
      (void)jacrev(
          [&](const std::vector<Trace>& xs) -> Trace {
            return fn_copy(xs);
          },
          params,
          opt.policy);
    }
    if (opt.vectorized) {
      auto [value, _] = jacfwd(
          [&](const std::vector<DualTrace>& xs) -> DualTrace {
            return fn_copy(xs);
          },
          params);
      const std::int64_t output_numel = value.numel();

      std::vector<std::vector<Series>> cols_by_param(params.size());
      for (std::size_t p = 0; p < params.size(); ++p) {
        cols_by_param[p].reserve(static_cast<std::size_t>(params[p].numel()));
        std::vector<std::vector<Series>> tangent_lanes;
        tangent_lanes.reserve(static_cast<std::size_t>(params[p].numel()));
        for (std::int64_t col = 0; col < params[p].numel(); ++col) {
          std::vector<Series> lane;
          lane.reserve(params.size());
          for (std::size_t q = 0; q < params.size(); ++q) {
            lane.push_back(
                q == p ? detail::flat_basis_like_(params[q], col)
                       : detail::zeros_like_(params[q]));
          }
          tangent_lanes.push_back(std::move(lane));
        }

        const auto [_, directional_lanes] = jvp_lanes(
            [&](const std::vector<DualTrace>& xs) -> DualTrace {
              return fn_copy(xs);
            },
            params,
            tangent_lanes);
        for (const auto& directional : directional_lanes) {
          cols_by_param[p].push_back(directional.reshape({directional.numel(), 1}));
        }
      }

      std::vector<Series> jacobians;
      jacobians.reserve(params.size());
      for (std::size_t p = 0; p < params.size(); ++p) {
        jacobians.push_back(detail::stack_cols_(
            cols_by_param[p], output_numel, value.scalar_type(), value.platform()));
      }
      return {value, jacobians};
    }
    return jacfwd(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(xs);
        },
        params);
  }
  if (opt.vectorized) {
    auto [value, _] = jacrev(
        [&](const std::vector<Trace>& xs) -> Trace {
          return fn_copy(xs);
        },
        params,
        opt.policy);
    const std::int64_t output_numel = value.numel();

    std::vector<Series> cotangent_lanes;
    cotangent_lanes.reserve(static_cast<std::size_t>(output_numel));
    for (std::int64_t row = 0; row < output_numel; ++row) {
      cotangent_lanes.push_back(detail::flat_basis_like_(value, row));
    }

    const auto [__, lane_grads] = vjp_lanes(
        [&](const std::vector<Trace>& xs) -> Trace {
          return fn_copy(xs);
        },
        params,
        cotangent_lanes,
        opt.policy);
    (void)__;

    std::vector<std::vector<Series>> rows_by_param(params.size());
    for (auto& rows : rows_by_param) rows.reserve(static_cast<std::size_t>(output_numel));
    for (const auto& lane : lane_grads) {
      for (std::size_t p = 0; p < lane.size(); ++p) {
        rows_by_param[p].push_back(lane[p].reshape({1, lane[p].numel()}));
      }
    }

    std::vector<Series> jacobians;
    jacobians.reserve(params.size());
    for (std::size_t p = 0; p < params.size(); ++p) {
      jacobians.push_back(detail::stack_rows_(
          rows_by_param[p], params[p].numel(), params[p].scalar_type(), params[p].platform()));
    }
    return {value, jacobians};
  }
  return jacrev(
      [&](const std::vector<Trace>& xs) -> Trace {
        return fn_copy(xs);
      },
      params,
      opt.policy);
}

template <class Fn>
std::pair<Series, Series> jacobian(
    Fn&& fn,
    const Series& param,
    const JacobianOptions& opt = {}) {
  if (opt.vectorized) {
    auto [value, jacobians] = jacobian(
        [&](const auto& xs) {
          return std::forward<Fn>(fn)(xs.front());
        },
        std::vector<Series>{param},
        opt);
    return {value, jacobians.front()};
  }
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  if (opt.mode == JacobianMode::Forward) {
    if (opt.policy.strict_dependency) {
      (void)jacrev(
          [&](const Trace& z) -> Trace {
            return fn_copy(z);
          },
          param,
          opt.policy);
    }
    return jacfwd(
        [&](const DualTrace& z) -> DualTrace {
          return fn_copy(z);
        },
        param);
  }
  return jacrev(
      [&](const Trace& z) -> Trace {
        return fn_copy(z);
      },
      param,
      opt.policy);
}

template <class Fn>
std::pair<Series, std::vector<Series>> hvp(Fn&& fn,
                                           const std::vector<Series>& params,
                                           const std::vector<Series>& tangents,
                                           const HvpOptions& opt = {},
                                           const DerivativePolicy& policy = {}) {
  if (params.size() != tangents.size()) {
    ::pluss::throw_error("subsp::math::diff::hvp: params/tangents size mismatch");
  }
  if (!(opt.epsilon > 0.0) || !std::isfinite(opt.epsilon)) {
    ::pluss::throw_error("subsp::math::diff::hvp: epsilon must be positive and finite");
  }

  for (std::size_t i = 0; i < params.size(); ++i) {
    detail::require_defined_(params[i], "hvp");
    detail::require_defined_(tangents[i], "hvp");
    detail::require_same_platform_(params[i], tangents[i], "hvp");
    detail::require_same_shape_(params[i], tangents[i], "hvp");
  }

  using FnType = std::decay_t<Fn>;
  if constexpr (std::is_invocable_v<FnType&, const std::vector<DualTrace>&>) {
    if (opt.mode == HvpOptions::Mode::FiniteDifferenceOnly) {
      ::pluss::throw_error(
          "subsp::math::diff::hvp: finite-difference-only mode requires a Trace-callable objective");
    }
    std::vector<Series> leaf_params;
    leaf_params.reserve(params.size());
    std::vector<Series> tangent_consts;
    tangent_consts.reserve(tangents.size());
    for (std::size_t i = 0; i < params.size(); ++i) {
      Series leaf = params[i].detach();
      detail::set_tracks_slope_(leaf, true);
      leaf_params.push_back(std::move(leaf));
      tangent_consts.push_back(tangents[i].detach());
    }

    FnType fn_copy(std::forward<Fn>(fn));
    const auto [value, directional] = jvp(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(xs);
        },
        leaf_params,
        tangent_consts);

    if (value.numel() != 1 || directional.numel() != 1) {
      ::pluss::throw_error("subsp::math::diff::hvp: objective must be scalar-valued");
    }

    if (!policy.accumulate_input_sensitivity) {
      for (const auto& leaf : leaf_params) {
        pluss::codem::series_tape::clear_sensitivity(leaf);
      }
    }
    ReversePolicy reverse_policy{};
    reverse_policy.keep_graph = policy.keep_graph;
    reverse_policy.accumulate_sensitivity = policy.accumulate_input_sensitivity;
    reverse_policy.track_higher_order = policy.track_higher_order;
    directional.reverse(
        reverse_policy.keep_graph,
        reverse_policy.accumulate_sensitivity,
        reverse_policy.track_higher_order);

    std::vector<Series> hv;
    hv.reserve(leaf_params.size());
    bool disconnected_warned = false;
    for (std::size_t i = 0; i < leaf_params.size(); ++i) {
      const auto& leaf = leaf_params[i];
      if (!leaf.has_sensitivity()) {
        if (policy.strict_dependency) {
          ::pluss::throw_error(
              "subsp::math::diff::hvp: objective is disconnected from input index " +
              std::to_string(i));
        }
        if (policy.warn_on_disconnected && !disconnected_warned) {
          PLUSS_WARN(
              "subsp::math::diff::hvp: objective disconnected from one or more inputs; "
              "materializing disconnected Hessian-vector sensitivities according to policy "
              "(set strict_dependency=true to fail loud).");
          disconnected_warned = true;
        }
        if (policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined) {
          hv.push_back(pluss::series::undefined());
          continue;
        }
        hv.push_back(detail::zeros_like_(leaf));
        continue;
      }
      hv.push_back(leaf.sensitivity());
    }
    return {value, hv};
  } else {
    if (opt.mode != HvpOptions::Mode::FiniteDifferenceOnly) {
      // Exact Hessian-vector product by reverse over reverse: differentiate g(x) . v, where g = grad f is itself
      // recorded on the tape.
      std::vector<Series> leaves;
      std::vector<Trace> vars;
      leaves.reserve(params.size());
      vars.reserve(params.size());
      for (const auto& p : params) {
        Series leaf = p.detach();
        detail::set_tracks_slope_(leaf, true);
        leaves.push_back(leaf);
        vars.emplace_back(leaf);
      }
      Trace output = detail::call_traced_(fn, vars);
      if (!output.defined()) {
        ::pluss::throw_error("subsp::math::diff::hvp: objective returned undefined output");
      }
      if (output.value().numel() != 1) {
        ::pluss::throw_error("subsp::math::diff::hvp: objective must be scalar-valued");
      }
      Series value = output.value();
      if (value.dim() != 0) value = value.reshape(std::vector<std::int64_t>{});

      std::vector<Series> hv;
      hv.reserve(params.size());
      if (detail::has_reverse_path_(output.value())) {
        output.value().reverse(/*keep_graph=*/true, /*accumulate_sensitivity=*/false, /*track_higher_order=*/true);
      }
      Series directional = pluss::series::undefined();
      for (std::size_t i = 0; i < leaves.size(); ++i) {
        if (!leaves[i].has_sensitivity()) continue;
        const Series part = leaves[i].sensitivity().mul(tangents[i].detach()).sum();
        directional = directional.defined() ? directional.add(part) : part;
      }
      for (const auto& leaf : leaves) pluss::codem::series_tape::clear_sensitivity(leaf);
      if (directional.defined() && detail::has_reverse_path_(directional)) {
        directional.reverse(policy.keep_graph, /*accumulate_sensitivity=*/false, policy.track_higher_order);
      }
      bool disconnected_warned = false;
      for (std::size_t i = 0; i < leaves.size(); ++i) {
        if (directional.defined() && leaves[i].has_sensitivity()) {
          hv.push_back(leaves[i].sensitivity());
          continue;
        }
        if (policy.strict_dependency) {
          ::pluss::throw_error(
              "subsp::math::diff::hvp: objective is disconnected from input index " + std::to_string(i));
        }
        if (policy.warn_on_disconnected && !disconnected_warned) {
          PLUSS_WARN(
              "subsp::math::diff::hvp: objective disconnected from one or more inputs; "
              "materializing disconnected Hessian-vector sensitivities according to policy "
              "(set strict_dependency=true to fail loud).");
          disconnected_warned = true;
        }
        hv.push_back(policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined
                         ? pluss::series::undefined()
                         : detail::zeros_like_(leaves[i]));
      }
      return {value, hv};
    }
    const auto [value, _] = value_and_grad(std::forward<Fn>(fn), params, policy);
    if (value.numel() != 1) {
      ::pluss::throw_error("subsp::math::diff::hvp: objective must be scalar-valued");
    }

    std::vector<Series> plus;
    std::vector<Series> minus;
    plus.reserve(params.size());
    minus.reserve(params.size());
    for (std::size_t i = 0; i < params.size(); ++i) {
      const Series delta = tangents[i].mul(opt.epsilon);
      plus.push_back(params[i].add(delta).detach());
      minus.push_back(params[i].sub(delta).detach());
    }

    const auto [plus_value, grad_plus] = value_and_grad(fn, plus, policy);
    const auto [minus_value, grad_minus] = value_and_grad(fn, minus, policy);
    (void)plus_value;
    (void)minus_value;

    std::vector<Series> hv;
    hv.reserve(params.size());
    const double scale = 1.0 / (2.0 * opt.epsilon);
    for (std::size_t i = 0; i < params.size(); ++i) {
      hv.push_back(grad_plus[i].sub(grad_minus[i]).mul(scale));
    }
    return {value, hv};
  }
}

template <class Fn>
std::pair<Series, std::vector<Series>> vhp(Fn&& fn,
                                           const std::vector<Series>& params,
                                           const std::vector<Series>& vectors,
                                           const HvpOptions& opt,
                                           const DerivativePolicy& policy) {
  if (params.size() != vectors.size()) {
    ::pluss::throw_error("subsp::math::diff::vhp: params/vectors size mismatch");
  }
  // For scalar objectives the Hessian is self-adjoint in exact arithmetic,
  // so VHP shares the same implementation surface as HVP.
  return hvp(std::forward<Fn>(fn), params, vectors, opt, policy);
}

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> hvp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& tangent_lanes,
    const HvpOptions& opt,
    const DerivativePolicy& policy) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));

  std::vector<std::vector<Series>> lanes;
  lanes.reserve(tangent_lanes.size());
  Series value = pluss::series::undefined();
  bool value_set = false;

  for (const auto& lane : tangent_lanes) {
    if (lane.size() != params.size()) {
      ::pluss::throw_error("subsp::math::diff::hvp_lanes: params/tangents size mismatch");
    }
    const auto [lane_value, lane_hv] = hvp(fn_copy, params, lane, opt, policy);
    if (!value_set) {
      value = lane_value;
      value_set = true;
    } else {
      detail::require_same_platform_(value, lane_value, "hvp_lanes");
      detail::require_same_shape_(value, lane_value, "hvp_lanes");
    }
    lanes.push_back(lane_hv);
  }

  if (!value_set) {
    ::pluss::throw_error(
        "subsp::math::diff::hvp_lanes: expected at least one tangent lane");
  }
  return {value, lanes};
}

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> vhp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& vector_lanes,
    const HvpOptions& opt,
    const DerivativePolicy& policy) {
  for (const auto& lane : vector_lanes) {
    if (lane.size() != params.size()) {
      ::pluss::throw_error("subsp::math::diff::vhp_lanes: params/vectors size mismatch");
    }
  }
  return hvp_lanes(std::forward<Fn>(fn), params, vector_lanes, opt, policy);
}

template <class Fn>
DerivativeAuditReport gradient_audit_report(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt) {
  DerivativeAuditReport report{};
  report.phase = "gradient";

  if (params.empty()) {
    ::pluss::throw_error("subsp::math::diff::gradient_audit_report: expected at least one parameter");
  }
  if (!(opt.epsilon > 0.0) || !std::isfinite(opt.epsilon)) {
    ::pluss::throw_error("subsp::math::diff::gradient_audit_report: epsilon must be positive and finite");
  }
  if (opt.atol < 0.0 || opt.rtol < 0.0 || !std::isfinite(opt.atol) || !std::isfinite(opt.rtol)) {
    ::pluss::throw_error("subsp::math::diff::gradient_audit_report: atol/rtol must be finite and non-negative");
  }
  for (const auto& p : params) {
    detail::require_defined_(p, "gradient_audit_report");
    detail::require_float_dtype_(p, "gradient_audit_report");
  }

  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  auto objective = [&](const auto& xs) {
    return fn_copy(xs);
  };

  auto primal_value = [&](const std::vector<Series>& xs) -> Series {
    std::vector<Trace> fixed_inputs;
    fixed_inputs.reserve(xs.size());
    for (const auto& x : xs) {
      fixed_inputs.emplace_back(trace(x, false));
    }
    Trace out = objective(fixed_inputs);
    if (!out.defined()) {
      ::pluss::throw_error("subsp::math::diff::gradient_audit_report: objective returned undefined Trace");
    }
    if (out.value().numel() != 1) {
      ::pluss::throw_error("subsp::math::diff::gradient_audit_report: objective must be scalar-valued");
    }
    return out.value();
  };

  const Series base_value = primal_value(params);

  const auto [_, grads] = value_and_grad(
      [&](const std::vector<Trace>& xs) -> Trace { return objective(xs); },
      params,
      opt.policy);
  (void)_;

  std::vector<std::vector<Series>> lanes;
  lanes.reserve(static_cast<std::size_t>(1 + (opt.include_axis_lanes ? params.size() : 0)));
  {
    std::vector<Series> global;
    global.reserve(params.size());
    for (const auto& p : params) global.push_back(detail::ones_like_(p));
    lanes.push_back(std::move(global));
  }
  if (opt.include_axis_lanes) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      std::vector<Series> lane;
      lane.reserve(params.size());
      for (std::size_t j = 0; j < params.size(); ++j) {
        lane.push_back(j == i ? detail::ones_like_(params[j]) : detail::zeros_like_(params[j]));
      }
      lanes.push_back(std::move(lane));
    }
  }

  for (std::size_t lane_index = 0; lane_index < lanes.size(); ++lane_index) {
    const auto& lane = lanes[lane_index];
    std::vector<Series> plus;
    std::vector<Series> minus;
    plus.reserve(params.size());
    minus.reserve(params.size());
    for (std::size_t i = 0; i < params.size(); ++i) {
      const Series delta = lane[i].mul(opt.epsilon);
      plus.push_back(params[i].add(delta).detach());
      minus.push_back(params[i].sub(delta).detach());
    }

    const Series f_plus = primal_value(plus);
    const Series f_minus = primal_value(minus);
    const double numeric = detail::scalar_item_(f_plus.sub(f_minus).div(2.0 * opt.epsilon), "gradient_audit");

    Series analytic_s = pluss::series::full({}, 0.0, base_value.scalar_type()).to(base_value.platform());
    for (std::size_t i = 0; i < grads.size(); ++i) {
      analytic_s = analytic_s.add(grads[i].mul(lane[i]).sum());
    }
    const double analytic = detail::scalar_item_(analytic_s, "gradient_audit_report");
    const double abs_error = std::abs(analytic - numeric);
    const double tolerance = opt.atol + opt.rtol * std::abs(numeric);
    if (!(std::isfinite(analytic) && std::isfinite(numeric)) || abs_error > tolerance) {
      report.passed = false;
      report.lane_index = static_cast<std::int64_t>(lane_index);
      report.analytic = analytic;
      report.numeric = numeric;
      report.abs_error = abs_error;
      report.tolerance = tolerance;
      return report;
    }
  }

  report.passed = true;
  report.abs_error = 0.0;
  report.tolerance = opt.atol;
  return report;
}

template <class Fn>
bool gradient_audit(Fn&& fn,
                    const std::vector<Series>& params,
                    const DerivativeAuditOptions& opt) {
  return gradient_audit_report(std::forward<Fn>(fn), params, opt).passed;
}

template <class Fn>
DerivativeAuditReport curvature_audit_report(
    Fn&& fn,
    const std::vector<Series>& params,
    const DerivativeAuditOptions& opt) {
  DerivativeAuditReport report{};
  report.phase = "curvature";

  if (params.empty()) {
    ::pluss::throw_error("subsp::math::diff::curvature_audit_report: expected at least one parameter");
  }
  if (!(opt.epsilon > 0.0) || !std::isfinite(opt.epsilon)) {
    ::pluss::throw_error("subsp::math::diff::curvature_audit_report: epsilon must be positive and finite");
  }
  if (opt.atol < 0.0 || opt.rtol < 0.0 || !std::isfinite(opt.atol) || !std::isfinite(opt.rtol)) {
    ::pluss::throw_error("subsp::math::diff::curvature_audit_report: atol/rtol must be finite and non-negative");
  }
  for (const auto& p : params) {
    detail::require_defined_(p, "curvature_audit_report");
    detail::require_float_dtype_(p, "curvature_audit_report");
  }

  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  auto objective = [&](const auto& xs) {
    return fn_copy(xs);
  };

  auto primal_value = [&](const std::vector<Series>& xs) -> Series {
    std::vector<Trace> fixed_inputs;
    fixed_inputs.reserve(xs.size());
    for (const auto& x : xs) {
      fixed_inputs.emplace_back(trace(x, false));
    }
    Trace out = objective(fixed_inputs);
    if (!out.defined()) {
      ::pluss::throw_error("subsp::math::diff::curvature_audit_report: objective returned undefined Trace");
    }
    if (out.value().numel() != 1) {
      ::pluss::throw_error("subsp::math::diff::curvature_audit_report: objective must be scalar-valued");
    }
    return out.value();
  };

  const Series base_value = primal_value(params);

  std::vector<std::vector<Series>> lanes;
  lanes.reserve(static_cast<std::size_t>(1 + (opt.include_axis_lanes ? params.size() : 0)));
  {
    std::vector<Series> global;
    global.reserve(params.size());
    for (const auto& p : params) global.push_back(detail::ones_like_(p));
    lanes.push_back(std::move(global));
  }
  if (opt.include_axis_lanes) {
    for (std::size_t i = 0; i < params.size(); ++i) {
      std::vector<Series> lane;
      lane.reserve(params.size());
      for (std::size_t j = 0; j < params.size(); ++j) {
        lane.push_back(j == i ? detail::ones_like_(params[j]) : detail::zeros_like_(params[j]));
      }
      lanes.push_back(std::move(lane));
    }
  }

  HvpOptions hvp_opt = opt.hvp;
  if (!(hvp_opt.epsilon > 0.0) || !std::isfinite(hvp_opt.epsilon)) {
    hvp_opt.epsilon = opt.epsilon;
  }

  for (std::size_t lane_index = 0; lane_index < lanes.size(); ++lane_index) {
    const auto& lane = lanes[lane_index];
    const auto [ignored_value, hv] = hvp(
        [&](const auto& xs) { return objective(xs); },
        params,
        lane,
        hvp_opt,
        opt.policy);
    (void)ignored_value;

    Series analytic_s = pluss::series::full({}, 0.0, base_value.scalar_type()).to(base_value.platform());
    for (std::size_t i = 0; i < hv.size(); ++i) {
      analytic_s = analytic_s.add(hv[i].mul(lane[i]).sum());
    }
    const double analytic = detail::scalar_item_(analytic_s, "curvature_audit_report");

    std::vector<Series> plus;
    std::vector<Series> minus;
    plus.reserve(params.size());
    minus.reserve(params.size());
    for (std::size_t i = 0; i < params.size(); ++i) {
      const Series delta = lane[i].mul(opt.epsilon);
      plus.push_back(params[i].add(delta).detach());
      minus.push_back(params[i].sub(delta).detach());
    }

    const Series f_plus = primal_value(plus);
    const Series f_minus = primal_value(minus);
    const double numeric = detail::scalar_item_(
        f_plus.sub(base_value.mul(2.0)).add(f_minus).div(opt.epsilon * opt.epsilon),
        "curvature_audit");

    const double abs_error = std::abs(analytic - numeric);
    const double tolerance = opt.atol + opt.rtol * std::abs(numeric);
    if (!(std::isfinite(analytic) && std::isfinite(numeric)) || abs_error > tolerance) {
      report.passed = false;
      report.lane_index = static_cast<std::int64_t>(lane_index);
      report.analytic = analytic;
      report.numeric = numeric;
      report.abs_error = abs_error;
      report.tolerance = tolerance;
      return report;
    }
  }
  report.passed = true;
  report.abs_error = 0.0;
  report.tolerance = opt.atol;
  return report;
}

template <class Fn>
bool curvature_audit(Fn&& fn,
                     const std::vector<Series>& params,
                     const DerivativeAuditOptions& opt) {
  return curvature_audit_report(std::forward<Fn>(fn), params, opt).passed;
}

template <class Fn>
std::pair<Series, std::function<Series(const Series&)>> linearize(Fn&& fn, const Series& param) {
  auto [value, pushforward] = [&] {
    if constexpr (std::is_invocable_v<std::decay_t<Fn>&, const DualTrace&>) {
      return linearize(
          [&](const std::vector<DualTrace>& xs) -> DualTrace {
            return std::forward<Fn>(fn)(xs.front());
          },
          std::vector<Series>{param});
    } else {
      return linearize(
          [&](const std::vector<Trace>& xs) -> Trace {
            return detail::call_traced_(std::forward<Fn>(fn), xs.front());
          },
          std::vector<Series>{param});
    }
  }();

  auto single_pushforward = [pushforward = std::move(pushforward)](const Series& tangent) -> Series {
    return pushforward(std::vector<Series>{tangent});
  };
  return std::pair{value, std::move(single_pushforward)};
}

template <class Fn>
std::pair<Series, Series> hvp(Fn&& fn,
                              const Series& param,
                              const Series& tangent,
                              const HvpOptions& opt = {},
                              const DerivativePolicy& policy = {}) {
  using FnType = std::decay_t<Fn>;
  if constexpr (std::is_invocable_v<FnType&, const DualTrace&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    auto [value, hv] = hvp(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(xs.front());
        },
        std::vector<Series>{param},
        std::vector<Series>{tangent},
        opt,
        policy);
    return {value, hv.front()};
  } else {
    auto [value, hv] = hvp(
        [&](const std::vector<Trace>& xs) -> Trace {
          return detail::call_traced_(std::forward<Fn>(fn), xs.front());
        },
        std::vector<Series>{param},
        std::vector<Series>{tangent},
        opt,
        policy);
    return {value, hv.front()};
  }
}

template <class Fn>
std::pair<Series, std::vector<std::vector<Series>>> hessian(
    Fn&& fn,
    const std::vector<Series>& params,
    const HessianOptions& opt = {}) {
  if (params.empty()) {
    ::pluss::throw_error("subsp::math::diff::hessian: expected at least one parameter");
  }
  for (const auto& p : params) {
    detail::require_defined_(p, "hessian");
  }

  std::vector<Series> zero_tangents;
  zero_tangents.reserve(params.size());
  for (const auto& p : params) {
    zero_tangents.push_back(detail::zeros_like_(p));
  }
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  const auto [value, _] = hvp(fn_copy, params, zero_tangents, opt.hvp, opt.policy);

  std::vector<std::vector<std::vector<Series>>> block_cols(
      params.size(),
      std::vector<std::vector<Series>>(params.size()));
  for (std::size_t i = 0; i < params.size(); ++i) {
    for (std::size_t j = 0; j < params.size(); ++j) {
      block_cols[i][j].reserve(static_cast<std::size_t>(params[j].numel()));
    }
  }

  for (std::size_t j = 0; j < params.size(); ++j) {
    std::vector<std::vector<Series>> tangent_lanes;
    tangent_lanes.reserve(static_cast<std::size_t>(params[j].numel()));
    for (std::int64_t col = 0; col < params[j].numel(); ++col) {
      std::vector<Series> lane;
      lane.reserve(params.size());
      for (std::size_t q = 0; q < params.size(); ++q) {
        lane.push_back(q == j ? detail::flat_basis_like_(params[q], col)
                              : detail::zeros_like_(params[q]));
      }
      tangent_lanes.push_back(std::move(lane));
    }

    if (opt.vectorized) {
      const auto [__, hv_lanes] = hvp_lanes(fn_copy, params, tangent_lanes, opt.hvp, opt.policy);
      (void)__;
      for (const auto& hv : hv_lanes) {
        for (std::size_t i = 0; i < params.size(); ++i) {
          block_cols[i][j].push_back(hv[i].reshape({hv[i].numel(), 1}));
        }
      }
      continue;
    }

    for (const auto& lane : tangent_lanes) {
      const auto [__, hv] = hvp(fn_copy, params, lane, opt.hvp, opt.policy);
      (void)__;
      for (std::size_t i = 0; i < params.size(); ++i) {
        block_cols[i][j].push_back(hv[i].reshape({hv[i].numel(), 1}));
      }
    }
  }

  std::vector<std::vector<Series>> blocks(
      params.size(),
      std::vector<Series>(params.size(), pluss::series::undefined()));
  for (std::size_t i = 0; i < params.size(); ++i) {
    for (std::size_t j = 0; j < params.size(); ++j) {
      blocks[i][j] = detail::stack_cols_(
          block_cols[i][j],
          params[i].numel(),
          params[i].scalar_type(),
          params[i].platform());
    }
  }
  return {value, blocks};
}

template <class Fn>
std::pair<Series, Series> hessian(Fn&& fn,
                                  const Series& param,
                                  const HessianOptions& opt = {}) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  std::pair<Series, std::vector<std::vector<Series>>> out{pluss::series::undefined(), {}};
  if constexpr (std::is_invocable_v<FnType&, const DualTrace&>) {
    out = hessian(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(xs.front());
        },
        std::vector<Series>{param},
        opt);
  } else {
    out = hessian(
        [&](const std::vector<Trace>& xs) -> Trace {
          return detail::call_traced_(fn_copy, xs.front());
        },
        std::vector<Series>{param},
        opt);
  }
  const auto& [value, blocks] = out;
  return {value, blocks.front().front()};
}

template <class Fn>
std::pair<Series, Series> hessian(Fn&& fn,
                                  const Series& param,
                                  const DerivativePolicy& policy) {
  HessianOptions opt{};
  opt.policy = policy;
  return hessian(std::forward<Fn>(fn), param, opt);
}

template <class Fn>
std::pair<Series, std::vector<Series>> hvp_lanes(
    Fn&& fn,
    const Series& param,
    const std::vector<Series>& tangent_lanes,
    const HvpOptions& opt = {},
    const DerivativePolicy& policy = {}) {
  std::vector<std::vector<Series>> lanes;
  lanes.reserve(tangent_lanes.size());
  for (const auto& t : tangent_lanes) {
    lanes.push_back(std::vector<Series>{t});
  }

  using FnType = std::decay_t<Fn>;
  if constexpr (std::is_invocable_v<FnType&, const DualTrace&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    auto [value, lane_hv] = hvp_lanes(
        [&](const std::vector<DualTrace>& xs) -> DualTrace {
          return fn_copy(xs.front());
        },
        std::vector<Series>{param},
        lanes,
        opt,
        policy);
    std::vector<Series> single;
    single.reserve(lane_hv.size());
    for (const auto& hv : lane_hv) single.push_back(hv.front());
    return {value, single};
  } else {
    auto [value, lane_hv] = hvp_lanes(
        [&](const std::vector<Trace>& xs) -> Trace {
          return detail::call_traced_(std::forward<Fn>(fn), xs.front());
        },
        std::vector<Series>{param},
        lanes,
        opt,
        policy);
    std::vector<Series> single;
    single.reserve(lane_hv.size());
    for (const auto& hv : lane_hv) single.push_back(hv.front());
    return {value, single};
  }
}

// Full derivative tensors use output axes first, followed by one copy of the
// input axes for every derivative order. Reverse passes produce one complete
// final input axis at a time, avoiding a redundant scalar pass for that axis.
template <class Fn>
Series nth_diff(Fn&& fn,
                const Series& param,
                std::int64_t order,
                const HvpOptions& opt = {},
                const DerivativePolicy& policy = {}) {
  detail::require_defined_(param, "nth_diff");
  if (order < 0) {
    ::pluss::throw_error("subsp::math::diff::nth_diff: order must be non-negative");
  }
  (void)opt;

  using FnType = std::decay_t<Fn>;
  if constexpr (std::is_invocable_v<FnType&, const Series&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    Series leaf = param.detach();
    detail::set_tracks_slope_(leaf, true);

    Series output = fn_copy(leaf);
    if (!output.defined()) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_diff: objective returned undefined output");
    }
    if (order == 0) return output;

    if (param.dim() > 0 &&
        order > (static_cast<std::int64_t>(pluss::codem::kMaxSeriesRank) -
                 output.dim()) /
                    param.dim()) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_diff: derivative tensor rank exceeds the "
          "64-dimensional Series limit; use JVP, VJP, or HVP products instead");
    }
    std::vector<std::int64_t> result_shape(output.sizes().begin(),
                                           output.sizes().end());
    for (std::int64_t level = 0; level < order; ++level) {
      result_shape.insert(result_shape.end(), param.sizes().begin(),
                          param.sizes().end());
    }
    if (output.numel() == 0 || param.numel() == 0) {
      return pluss::series::empty(result_shape, param.scalar_type())
          .to(param.platform());
    }

    std::vector<Series> branches;
    branches.reserve(static_cast<std::size_t>(output.numel()));
    const Series flat_output = output.reshape({output.numel()});
    for (std::int64_t lane = 0; lane < output.numel(); ++lane) {
      branches.push_back(flat_output.select(0, lane));
    }

    bool warned_disconnected = false;
    for (std::int64_t level = 1; level <= order; ++level) {
      const bool interior = level < order;
      std::vector<Series> next;
      std::vector<Series> rows;
      if (interior) {
        next.reserve(static_cast<std::size_t>(branches.size()) *
                     static_cast<std::size_t>(param.numel()));
      } else {
        rows.reserve(branches.size());
      }

      for (const Series& branch : branches) {
        pluss::codem::series_tape::clear_sensitivity(leaf);
        if (detail::has_reverse_path_(branch)) {
          branch.reverse(/*keep_graph=*/policy.keep_graph || interior ||
                             branches.size() > 1,
                         /*accumulate_sensitivity=*/false,
                         /*track_higher_order=*/policy.track_higher_order ||
                             interior);
        }

        Series gradient = pluss::series::undefined();
        if (leaf.has_sensitivity()) {
          gradient = leaf.sensitivity();
        } else {
          if (level == 1 && policy.strict_dependency) {
            ::pluss::throw_error(
                "subsp::math::diff::nth_diff: an output lane is disconnected "
                "from the input");
          }
          if (level == 1 && policy.warn_on_disconnected &&
              !warned_disconnected) {
            PLUSS_WARN(
                "subsp::math::diff::nth_diff: one or more output lanes are "
                "disconnected from the input; materializing zeros according "
                "to policy (set strict_dependency=true to fail loud).");
            warned_disconnected = true;
          }
          if (policy.disconnected_sensitivity ==
              DisconnectedSensitivity::Undefined) {
            return pluss::series::undefined();
          }
          gradient = detail::zeros_like_(leaf);
        }

        if (interior) {
          const Series flat_gradient = gradient.reshape({param.numel()});
          for (std::int64_t lane = 0; lane < param.numel(); ++lane) {
            next.push_back(flat_gradient.select(0, lane));
          }
        } else {
          rows.push_back(gradient.reshape({1, param.numel()}));
        }
      }

      if (interior) {
        branches = std::move(next);
      } else {
        return detail::stack_rows_(rows, param.numel(), param.scalar_type(),
                                   param.platform())
            .reshape(result_shape);
      }
    }
    return pluss::series::undefined();
  } else if constexpr (std::is_invocable_v<FnType&, const Trace&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    auto series_callback = [&fn_copy](const Series& value) {
      Trace output = fn_copy(Trace(value));
      if (!output.defined()) {
        ::pluss::throw_error(
            "subsp::math::diff::nth_diff: objective returned undefined Trace");
      }
      return output.value();
    };
    return nth_diff(series_callback, param, order, opt, policy);
  } else if constexpr (std::is_invocable_v<FnType&, const DualTrace&>) {
    if (order == 0) {
      const Series tangent = detail::zeros_like_(param);
      return jvp(std::forward<Fn>(fn), param, tangent).first;
    }
    if (order == 1) {
      auto [value, derivative] = jacfwd(std::forward<Fn>(fn), param);
      std::vector<std::int64_t> shape(value.sizes().begin(), value.sizes().end());
      shape.insert(shape.end(), param.sizes().begin(), param.sizes().end());
      return derivative.reshape(shape);
    }
    ::pluss::throw_error(
        "subsp::math::diff::nth_diff: order > 1 requires a Trace-callable or "
        "Series-callable objective");
  } else {
    ::pluss::throw_error(
        "subsp::math::diff::nth_diff: objective must be callable with Trace, Series, or DualTrace");
  }
}

// Treat several independently shaped Series as one ordered coordinate space.
// Every derivative axis has length sum(params[i].numel()); coordinates are
// assigned by argument order, then row-major flat position within an argument.
// Packing and reconstruction use Series operations so the implementation stays
// on the selected backend and delegates all derivative traversal to the
// canonical single-input engine above.
template <class Fn>
Series nth_diff(Fn&& fn,
                const std::vector<Series>& params,
                std::int64_t order,
                const HvpOptions& opt = {},
                const DerivativePolicy& policy = {}) {
  if (params.empty()) {
    ::pluss::throw_error(
        "subsp::math::diff::nth_diff: expected at least one independent Series");
  }

  const auto platform = params.front().platform();
  ScalarType joint_dtype = params.front().scalar_type();
  std::int64_t joint_size = 0;
  std::vector<std::vector<std::int64_t>> shapes;
  std::vector<ScalarType> dtypes;
  std::vector<std::int64_t> offsets;
  shapes.reserve(params.size());
  dtypes.reserve(params.size());
  offsets.reserve(params.size() + 1);
  offsets.push_back(0);

  for (const Series& param : params) {
    detail::require_defined_(param, "nth_diff");
    detail::require_float_dtype_(param, "nth_diff");
    if (param.platform() != platform) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_diff: all independent Series must use the same platform");
    }
    if (param.numel() > std::numeric_limits<std::int64_t>::max() - joint_size) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_diff: joint coordinate count overflows int64");
    }
    joint_size += param.numel();
    joint_dtype = promote_types(joint_dtype, param.scalar_type());
    shapes.emplace_back(param.sizes().begin(), param.sizes().end());
    dtypes.push_back(param.scalar_type());
    offsets.push_back(joint_size);
  }

  std::vector<Series> flat_parts;
  flat_parts.reserve(params.size());
  for (const Series& param : params) {
    if (param.numel() == 0) continue;
    Series flat = param.reshape({param.numel()});
    if (flat.scalar_type() != joint_dtype) flat = flat.to(joint_dtype);
    flat_parts.push_back(std::move(flat));
  }

  Series joint = pluss::series::empty({0}, joint_dtype).to(platform);
  if (!flat_parts.empty()) {
    joint = flat_parts.front();
    if (flat_parts.size() > 1) {
      joint = joint.unite(
          std::vector<Series>(flat_parts.begin() + 1, flat_parts.end()), 0);
    }
  }

  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  auto packed_fn = [fn_copy = std::move(fn_copy), shapes = std::move(shapes),
                    dtypes = std::move(dtypes), offsets = std::move(offsets)](
                       const Series& packed) mutable -> Series {
    std::vector<Series> unpacked;
    unpacked.reserve(shapes.size());
    for (std::size_t i = 0; i < shapes.size(); ++i) {
      Series value = packed.slice(0, offsets[i], offsets[i + 1]).reshape(shapes[i]);
      if (value.scalar_type() != dtypes[i]) value = value.to(dtypes[i]);
      unpacked.push_back(std::move(value));
    }
    return fn_copy(unpacked);
  };

  return nth_diff(std::move(packed_fn), joint, order, opt, policy);
}

template <class Fn>
auto nth_diff(Fn&& fn,
              std::int64_t order,
              const HvpOptions& opt = {},
              const DerivativePolicy& policy = {}) {
  using FnType = std::decay_t<Fn>;
  return [fn_copy = FnType(std::forward<Fn>(fn)), order, opt, policy](const Series& param) mutable -> Series {
    return nth_diff(fn_copy, param, order, opt, policy);
  };
}

template <class Fn>
std::pair<Series, std::function<Series(const Series&)>> vjp(
    Fn&& fn, const Series& param, const DerivativePolicy& policy = {}) {
  std::vector<Series> params{param};
  auto [value, pullback] = vjp(
      [&](const std::vector<Trace>& xs) -> Trace {
        return detail::call_traced_(std::forward<Fn>(fn), xs.front());
      },
      params,
      policy);

  auto single_pullback = [pullback = std::move(pullback)](const Series& cotangent) mutable -> Series {
    return pullback(cotangent).front();
  };
  return {value, std::move(single_pullback)};
}

template <class Fn>
std::pair<Series, Series> vjp(Fn&& fn,
                              const Series& param,
                              const Series& cotangent,
                              const DerivativePolicy& policy = {}) {
  auto [value, pullback] = vjp(std::forward<Fn>(fn), param, policy);
  return {value, pullback(cotangent)};
}

template <class Fn>
std::pair<Series, std::vector<Series>> vjp_lanes(
    Fn&& fn,
    const Series& param,
    const std::vector<Series>& cotangent_lanes,
    const DerivativePolicy& policy = {}) {
  std::vector<Series> params{param};
  auto [value, lanes] = vjp_lanes(
      [&](const std::vector<Trace>& xs) -> Trace {
        return detail::call_traced_(std::forward<Fn>(fn), xs.front());
      },
      params,
      cotangent_lanes,
      policy);
  std::vector<Series> single_lanes;
  single_lanes.reserve(lanes.size());
  for (const auto& lane : lanes) {
    single_lanes.push_back(lane.front());
  }
  return {value, single_lanes};
}

template <class Fn>
std::pair<Series, Series> vhp(Fn&& fn,
                              const Series& param,
                              const Series& vector,
                              const HvpOptions& opt = {},
                              const DerivativePolicy& policy = {}) {
  return hvp(std::forward<Fn>(fn), param, vector, opt, policy);
}

template <class Fn>
std::pair<Series, std::vector<Series>> vhp_lanes(
    Fn&& fn,
    const Series& param,
    const std::vector<Series>& vector_lanes,
    const HvpOptions& opt = {},
    const DerivativePolicy& policy = {}) {
  return hvp_lanes(std::forward<Fn>(fn), param, vector_lanes, opt, policy);
}

template <class Fn>
bool gradient_audit(Fn&& fn,
                    const Series& param,
                    const DerivativeAuditOptions& opt = {}) {
  return gradient_audit_report(std::forward<Fn>(fn), param, opt).passed;
}

template <class Fn>
DerivativeAuditReport gradient_audit_report(
    Fn&& fn,
    const Series& param,
    const DerivativeAuditOptions& opt) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  return gradient_audit_report(
      [&](const auto& xs) { return fn_copy(xs.front()); },
      std::vector<Series>{param},
      opt);
}

template <class Fn>
bool curvature_audit(Fn&& fn,
                     const Series& param,
                     const DerivativeAuditOptions& opt = {}) {
  return curvature_audit_report(std::forward<Fn>(fn), param, opt).passed;
}

template <class Fn>
DerivativeAuditReport curvature_audit_report(
    Fn&& fn,
    const Series& param,
    const DerivativeAuditOptions& opt) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  return curvature_audit_report(
      [&](const auto& xs) { return fn_copy(xs.front()); },
      std::vector<Series>{param},
      opt);
}

template <class Fn>
std::pair<Series, std::vector<Series>> jvp_lanes(
    Fn&& fn,
    const Series& param,
    const std::vector<Series>& tangent_lanes) {
  std::vector<Series> params{param};
  std::vector<std::vector<Series>> lanes;
  lanes.reserve(tangent_lanes.size());
  for (const auto& t : tangent_lanes) {
    lanes.push_back(std::vector<Series>{t});
  }
  return jvp_lanes(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(xs.front());
      },
      params,
      lanes);
}

} // namespace pluss::subsp::math::diff
