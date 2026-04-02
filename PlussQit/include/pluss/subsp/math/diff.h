#pragma once

#include "pluss/codem/series_factory.h"
#include "CodeM/compat/Invoke.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/SeriesTape.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
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
  return pluss::full_like(like, 0.0, like.scalar_type());
}

inline Series ones_like_(const Series& like) {
  return pluss::full_like(like, 1.0, like.scalar_type());
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
  Series basis = pluss::arange(0, like.numel(), 1, ScalarType::Int64).to(like.platform());
  basis = basis.eq(index).to(like.scalar_type());
  return basis.reshape(like.sizes());
}

inline Series stack_rows_(const std::vector<Series>& rows,
                          std::int64_t ncols,
                          ScalarType dtype,
                          pluss::codem::Platform platform) {
  if (rows.empty()) return pluss::empty({0, ncols}, dtype).to(platform);
  if (rows.size() == 1) return rows.front();
  return rows.front().unite(std::vector<Series>(rows.begin() + 1, rows.end()), 0);
}

inline Series stack_cols_(const std::vector<Series>& cols,
                          std::int64_t nrows,
                          ScalarType dtype,
                          pluss::codem::Platform platform) {
  if (cols.empty()) return pluss::empty({nrows, 0}, dtype).to(platform);
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
    return value_->is_leaf().item<bool>();
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

  void clear_sensitivity() {
    if (defined()) value_->clear_sensitivity();
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
  return fixed(pluss::full_like(like.value(), value, like.value().scalar_type()));
}

inline DualTrace make_scalar_like_(const DualTrace& like, double value) {
  return DualTrace(
      pluss::full_like(like.value(), value, like.value().scalar_type()),
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

inline Trace exp(const Trace& x) {
  detail::require_defined_(x.value(), "exp");
  return Trace(x.value().exp());
}

inline DualTrace exp(const DualTrace& x) {
  const Series primal = x.value().exp();
  return DualTrace(primal, primal.mul(x.tangent()));
}

inline Trace log(const Trace& x) {
  detail::require_defined_(x.value(), "log");
  return Trace(x.value().log());
}

inline DualTrace log(const DualTrace& x) {
  return DualTrace(x.value().log(), x.tangent().div(x.value()));
}

inline Trace sqrt(const Trace& x) {
  detail::require_defined_(x.value(), "sqrt");
  return Trace(x.value().sqrt());
}

inline DualTrace sqrt(const DualTrace& x) {
  const Series primal = x.value().sqrt();
  return DualTrace(primal, x.tangent().mul(0.5).mul(x.value().rsqrt()));
}

inline Trace sin(const Trace& x) {
  detail::require_defined_(x.value(), "sin");
  return Trace(x.value().sin());
}

inline DualTrace sin(const DualTrace& x) {
  return DualTrace(x.value().sin(), x.value().cos().mul(x.tangent()));
}

inline Trace cos(const Trace& x) {
  detail::require_defined_(x.value(), "cos");
  return Trace(x.value().cos());
}

inline DualTrace cos(const DualTrace& x) {
  return DualTrace(x.value().cos(), x.value().sin().neg().mul(x.tangent()));
}

inline Trace tanh(const Trace& x) {
  detail::require_defined_(x.value(), "tanh");
  return Trace(x.value().tanh());
}

inline DualTrace tanh(const DualTrace& x) {
  const Series primal = x.value().tanh();
  const Series slope = detail::ones_like_(primal).sub(primal.square());
  return DualTrace(primal, slope.mul(x.tangent()));
}

inline Trace asin(const Trace& x) {
  detail::require_defined_(x.value(), "asin");
  return Trace(x.value().asin());
}

inline DualTrace asin(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  const Series denom = one.sub(x.value().square()).sqrt();
  return DualTrace(x.value().asin(), x.tangent().div(denom));
}

inline Trace acos(const Trace& x) {
  detail::require_defined_(x.value(), "acos");
  return Trace(x.value().acos());
}

inline DualTrace acos(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  const Series denom = one.sub(x.value().square()).sqrt();
  return DualTrace(x.value().acos(), x.tangent().div(denom).neg());
}

inline Trace atan(const Trace& x) {
  detail::require_defined_(x.value(), "atan");
  return Trace(x.value().atan());
}

inline DualTrace atan(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  return DualTrace(x.value().atan(), x.tangent().div(one.add(x.value().square())));
}

inline Trace tan(const Trace& x) {
  detail::require_defined_(x.value(), "tan");
  return Trace(x.value().tan());
}

inline DualTrace tan(const DualTrace& x) {
  const Series c = x.value().cos();
  const Series slope = detail::ones_like_(c).div(c.square());
  return DualTrace(x.value().tan(), slope.mul(x.tangent()));
}

inline Trace sinh(const Trace& x) {
  detail::require_defined_(x.value(), "sinh");
  return Trace(x.value().sinh());
}

inline DualTrace sinh(const DualTrace& x) {
  return DualTrace(x.value().sinh(), x.value().cosh().mul(x.tangent()));
}

inline Trace cosh(const Trace& x) {
  detail::require_defined_(x.value(), "cosh");
  return Trace(x.value().cosh());
}

inline DualTrace cosh(const DualTrace& x) {
  return DualTrace(x.value().cosh(), x.value().sinh().mul(x.tangent()));
}

inline Trace asinh(const Trace& x) {
  detail::require_defined_(x.value(), "asinh");
  return Trace(x.value().asinh());
}

inline DualTrace asinh(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  const Series slope = one.div(one.add(x.value().square()).sqrt());
  return DualTrace(x.value().asinh(), slope.mul(x.tangent()));
}

inline Trace acosh(const Trace& x) {
  detail::require_defined_(x.value(), "acosh");
  return Trace(x.value().acosh());
}

inline DualTrace acosh(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  const Series slope = one.div(x.value().sub(one).sqrt().mul(x.value().add(one).sqrt()));
  return DualTrace(x.value().acosh(), slope.mul(x.tangent()));
}

inline Trace atanh(const Trace& x) {
  detail::require_defined_(x.value(), "atanh");
  return Trace(x.value().atanh());
}

inline DualTrace atanh(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  const Series slope = one.div(one.sub(x.value().square()));
  return DualTrace(x.value().atanh(), slope.mul(x.tangent()));
}

inline Trace square(const Trace& x) {
  detail::require_defined_(x.value(), "square");
  return Trace(x.value().square());
}

inline DualTrace square(const DualTrace& x) {
  return DualTrace(x.value().square(), x.value().mul(2.0).mul(x.tangent()));
}

inline Trace rsqrt(const Trace& x) {
  detail::require_defined_(x.value(), "rsqrt");
  return Trace(x.value().rsqrt());
}

inline DualTrace rsqrt(const DualTrace& x) {
  const Series primal = x.value().rsqrt();
  const Series slope = primal.square().mul(primal).mul(-0.5);
  return DualTrace(primal, slope.mul(x.tangent()));
}

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

inline Trace log1p(const Trace& x) {
  detail::require_defined_(x.value(), "log1p");
  return Trace(x.value().log1p());
}

inline DualTrace log1p(const DualTrace& x) {
  const Series one = detail::ones_like_(x.value());
  return DualTrace(x.value().log1p(), x.tangent().div(one.add(x.value())));
}

inline Trace expm1(const Trace& x) {
  detail::require_defined_(x.value(), "expm1");
  return Trace(x.value().expm1());
}

inline DualTrace expm1(const DualTrace& x) {
  return DualTrace(x.value().expm1(), x.value().exp().mul(x.tangent()));
}

inline Trace erf(const Trace& x) {
  detail::require_defined_(x.value(), "erf");
  return Trace(x.value().erf());
}

inline DualTrace erf(const DualTrace& x) {
  constexpr double kTwoOverSqrtPi = 1.12837916709551257390;
  const Series slope = x.value().square().neg().exp().mul(kTwoOverSqrtPi);
  return DualTrace(x.value().erf(), slope.mul(x.tangent()));
}

inline Trace relu(const Trace& x) {
  detail::require_defined_(x.value(), "relu");
  return Trace(x.value().relu());
}

inline DualTrace relu(const DualTrace& x) {
  const Series mask = x.value().gt(0.0).to(x.value().scalar_type());
  return DualTrace(x.value().relu(), mask.mul(x.tangent()));
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

inline Trace sigmoid(const Trace& x) {
  detail::require_defined_(x.value(), "sigmoid");
  return Trace(x.value().sigmoid());
}

inline DualTrace sigmoid(const DualTrace& x) {
  const Series primal = x.value().sigmoid();
  const Series slope = primal.mul(detail::ones_like_(primal).sub(primal));
  return DualTrace(primal, slope.mul(x.tangent()));
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
    p.clear_sensitivity();
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

  Trace loss = std::forward<Fn>(fn)(vars);
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
  loss.reverse(reverse_policy);

  std::vector<Series> grads;
  grads.reserve(vars.size());
  for (std::size_t i = 0; i < vars.size(); ++i) {
    const auto& v = vars[i];
    if (!v.has_sensitivity()) {
      if (policy.strict_dependency) {
        ::pluss::throw_error(
            "subsp::math::diff::value_and_grad: objective is disconnected from input index " +
            std::to_string(i));
      }
      if (policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined) {
        grads.emplace_back();
        continue;
      }
    }
    grads.push_back(v.sensitivity());
  }
  return {loss.value(), grads};
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

template <class Fn>
std::pair<Series, std::vector<Series>> jvp_lanes(
    Fn&& fn,
    const std::vector<Series>& params,
    const std::vector<std::vector<Series>>& tangent_lanes) {
  std::vector<Series> lanes;
  lanes.reserve(tangent_lanes.size());
  Series value;
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

template <class Fn>
std::pair<Series, std::vector<Series>> jacfwd(Fn&& fn, const std::vector<Series>& params) {
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

  Trace output = std::forward<Fn>(fn)(vars);
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
    output.reverse(cotangent, reverse_policy);
    pullback_consumed = true;

    std::vector<Series> grads;
    grads.reserve(vars.size());
    for (std::size_t i = 0; i < vars.size(); ++i) {
      const auto& v = vars[i];
      if (!v.has_sensitivity()) {
        if (policy.strict_dependency) {
          ::pluss::throw_error(
              "subsp::math::diff::vjp: output is disconnected from input index " +
              std::to_string(i));
        }
        if (policy.disconnected_sensitivity == DisconnectedSensitivity::Undefined) {
          grads.emplace_back();
          continue;
        }
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
        return std::forward<Fn>(fn)(xs.front());
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
        return std::forward<Fn>(fn)(xs.front());
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
        return std::forward<Fn>(fn)(xs.front());
      },
      std::vector<Series>{param},
      policy);
  return {value, jacobians.front()};
}

template <class Fn>
std::pair<Series, Series> jvp(Fn&& fn, const Series& param, const Series& tangent) {
  return jvp(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(xs.front());
      },
      std::vector<Series>{param},
      std::vector<Series>{tangent});
}

template <class Fn>
std::pair<Series, Series> jacfwd(Fn&& fn, const Series& param) {
  auto [value, jacobians] = jacfwd(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(xs.front());
      },
      std::vector<Series>{param});
  return {value, jacobians.front()};
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
    for (std::size_t i = 0; i < leaf_params.size(); ++i) {
      const auto& leaf = leaf_params[i];
      if (policy.strict_dependency && !leaf.has_sensitivity()) {
        ::pluss::throw_error(
            "subsp::math::diff::hvp: objective is disconnected from input index " +
            std::to_string(i));
      }
      hv.push_back(leaf.sensitivity());
    }
    return {value, hv};
  } else {
    if (opt.mode == HvpOptions::Mode::ExactOnly) {
      ::pluss::throw_error(
          "subsp::math::diff::hvp: exact-only mode requires a DualTrace-callable objective");
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
  Series value;
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

    Series analytic_s = pluss::full({}, 0.0, base_value.scalar_type()).to(base_value.platform());
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

    Series analytic_s = pluss::full({}, 0.0, base_value.scalar_type()).to(base_value.platform());
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
  auto [value, pushforward] = linearize(
      [&](const std::vector<DualTrace>& xs) -> DualTrace {
        return std::forward<Fn>(fn)(xs.front());
      },
      std::vector<Series>{param});

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
          return std::forward<Fn>(fn)(xs.front());
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

  std::vector<std::vector<Series>> blocks(params.size(), std::vector<Series>(params.size()));
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
  std::pair<Series, std::vector<std::vector<Series>>> out;
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
          return fn_copy(xs.front());
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
          return std::forward<Fn>(fn)(xs.front());
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

// Scalar-univariate convenience helper built on top of the transform layer.
// This intentionally does not overload `reverse()` with an order argument,
// because higher-order derivatives need explicit transform semantics.
template <class Fn>
Series nth_derivative(Fn&& fn,
                      const Series& param,
                      std::int64_t order,
                      const HvpOptions& opt = {}) {
  detail::require_defined_(param, "nth_derivative");
  if (order < 0) {
    ::pluss::throw_error("subsp::math::diff::nth_derivative: order must be non-negative");
  }
  if (param.numel() != 1) {
    ::pluss::throw_error(
        "subsp::math::diff::nth_derivative: currently requires a scalar input Series");
  }

  using FnType = std::decay_t<Fn>;
  if constexpr (std::is_invocable_v<FnType&, const Trace&>) {
    FnType fn_copy(std::forward<Fn>(fn));
    Series leaf = param.detach();
    detail::set_tracks_slope_(leaf, true);
    Trace out = fn_copy(Trace(leaf));
    if (!out.defined()) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_derivative: objective returned undefined Trace");
    }
    if (out.value().numel() != 1) {
      ::pluss::throw_error(
          "subsp::math::diff::nth_derivative: objective must return scalar output");
    }

    if (order == 0) return out.value();

    Series current = out.value();
    for (std::int64_t k = 1; k <= order; ++k) {
      pluss::codem::series_tape::clear_sensitivity(leaf);
      const bool need_higher_graph = (k < order);
      current.reverse(
          /*keep_graph=*/true,
          /*accumulate_sensitivity=*/false,
          /*track_higher_order=*/need_higher_graph);

      if (!leaf.has_sensitivity()) {
        return detail::zeros_like_(leaf);
      }
      current = leaf.sensitivity();
    }
    return current;
  } else if constexpr (std::is_invocable_v<FnType&, const DualTrace&>) {
    if (order == 0) {
      const Series tangent = detail::zeros_like_(param);
      return jvp(std::forward<Fn>(fn), param, tangent).first;
    }
    if (order == 1) {
      return grad(
          [&](const Trace& z) -> Trace {
            return fn(trace(z.value(), detail::zeros_like_(z.value()))).value();
          },
          param);
    }
    if (order == 2) {
      const Series tangent = detail::ones_like_(param);
      return hvp(std::forward<Fn>(fn), param, tangent, opt).second;
    }
    ::pluss::throw_error(
        "subsp::math::diff::nth_derivative: order > 2 requires a Trace-callable objective");
  } else {
    ::pluss::throw_error(
        "subsp::math::diff::nth_derivative: objective must be callable with Trace or DualTrace");
  }
}

template <class Fn>
std::pair<Series, std::function<Series(const Series&)>> vjp(
    Fn&& fn, const Series& param, const DerivativePolicy& policy = {}) {
  std::vector<Series> params{param};
  auto [value, pullback] = vjp(
      [&](const std::vector<Trace>& xs) -> Trace {
        return std::forward<Fn>(fn)(xs.front());
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
        return std::forward<Fn>(fn)(xs.front());
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
